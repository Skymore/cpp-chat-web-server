/*
 * @Author       : Rui Tao
 * @Date         : 2024-04-18
 * @copyleft Apache 2.0
 */
#include "httprequest.h"

using namespace std;

BloomFilter<UINT32_MAX> HttpRequest::bloomFilter;

const unordered_set <string> HttpRequest::DEFAULT_HTML{
        "/index", "/register", "/login",
        "/welcome", "/video", "/picture",};

const unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG{
        {"/register.html", 0},
        {"/login.html",    1},};

void HttpRequest::Init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
    bloom_init();
}

void HttpRequest::bloom_init() {
    SqlConnPool *pool = SqlConnPool::Instance();  // 获取连接池实例
    MYSQL *mysql = pool->GetConn();  // 从连接池中获取一个连接
    if (!mysql) {
        LOG_ERROR("Failed to get a database connection");
        return;
    }

    const char *query = "SELECT username FROM user";  // 查询所有用户名
    if (mysql_query(mysql, query)) {
        LOG_ERROR("Query Error: %s", mysql_error(mysql));
        pool->FreeConn(mysql);  // 查询失败，释放连接
        return;
    }

    MYSQL_RES *result = mysql_store_result(mysql);
    if (!result) {
        LOG_ERROR("Store Result Error: %s", mysql_error(mysql));
        pool->FreeConn(mysql);  // 获取结果集失败，释放连接
        return;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr) {
        if (row[0]) {
            bloomFilter.add(row[0]);  // 将用户名添加到布隆过滤器
        }
    }

    mysql_free_result(result);  // 释放结果集
    pool->FreeConn(mysql);  // 释放数据库连接
}


bool HttpRequest::IsKeepAlive() const {
    if (header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::parse(Buffer &buff) {
    const char CRLF[] = "\r\n";
    if (buff.ReadableBytes() <= 0) {
        return false;
    }
    while (buff.ReadableBytes() && state_ != FINISH) {
        const char *lineEnd = search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
        // 解释: search() 函数在 [buff.Peek(), buff.BeginWriteConst()) 中查找 CRLF，返回第一次出现的位置
        // 如果没找到，返回 buff.BeginWriteConst()
        // 如果找到，返回 CRLF 的首地址
        // CRLF 是"\r\n"，长度为2, 所以 CRLF + 2 是指向 "\n" 的地址
        std::string line(buff.Peek(), lineEnd);
        switch (state_) {
            case REQUEST_LINE:
                cout << "requ: " << line << endl;
                if (!ParseRequestLine_(line)) {
                    return false;
                }
                ParsePath_();
                break;
            case HEADERS:
                cout << "head: " << line << endl;
                // 解析头部...
                if (line.empty()) {  // 头部结束
                    if (method_ == "POST") {
                        state_ = BODY;
                    } else {
                        state_ = FINISH;  // 无内容的 POST 请求或错误情况
                    }
                } else {
                    ParseHeader_(line);
                }
                break;
            case BODY: {
                cout << "body: " << line << endl;
                size_t contentLength_ = stoi(header_["Content-Length"]);
                if (contentLength_ > 0) {
                    if (buff.ReadableBytes() < contentLength_) {
                        break;
                    }
                    ParseBody_(line);
                    buff.Retrieve(contentLength_);
                    state_ = FINISH;
                } else {
                    state_ = FINISH;
                }
                break;
            }
            default:
                cout << "defa: " << line << endl;
                break;
        }
        if (lineEnd == buff.BeginWrite()) {
            break;
        }
        buff.RetrieveUntil(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}


void HttpRequest::ParsePath_() {
    if (path_ == "/") {
        path_ = "/index.html";
    } else {
        for (auto &item: DEFAULT_HTML) {
            if (item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

bool HttpRequest::ParseRequestLine_(const string &line) {
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch subMatch;
    if (regex_match(line, subMatch, patten)) {
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::ParseHeader_(const string &line) {
    regex patten("^([^:]*): ?(.*)$");
    smatch subMatch;
    if (regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
    } else {
        state_ = BODY;
    }
}

void HttpRequest::ParseBody_(const string &line) {
    body_ = line;
    ParsePost_();
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

int HttpRequest::ConverHex(char ch) {
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return ch;
}

void HttpRequest::ParsePost_() {
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        ParseFromUrlencoded_();
        if (DEFAULT_HTML_TAG.count(path_)) {
            cout << "login or register" << endl;
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            if (tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if (UserVerify(post_["username"], post_["password"], isLogin)) {
                    path_ = "/welcome.html";
                } else {
                    path_ = "/error.html";
                }
            }
        }
    }
}

void HttpRequest::ParseFromUrlencoded_() {
    if (body_.size() == 0) { return; }

    string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for (; i < n; i++) {
        char ch = body_[i];
        switch (ch) {
            case '=':
                key = body_.substr(j, i - j);
                j = i + 1;
                break;
            case '+':
                body_[i] = ' ';
                break;
            case '%':
                num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
                body_[i + 2] = num % 10 + '0';
                body_[i + 1] = num / 10 + '0';
                i += 2;
                break;
            case '&':
                value = body_.substr(j, i - j);
                j = i + 1;
                post_[key] = value;
                LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
                break;
            default:
                break;
        }
    }
    assert(j <= i);
    if (post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

bool HttpRequest::UserVerify(const string &name, const string &pwd, bool isLogin) {
    if (name == "" || pwd == "") { return false; }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());

    // 使用布隆过滤器首先检查用户名是否可能存在
    if (!bloomFilter.possiblyContains(name)) {
        LOG_DEBUG("Bloom filter says username does not exist.");
        return false;
    }

    MYSQL *sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);

    bool flag = false;
    char order[256] = {0};
    MYSQL_RES *res = nullptr;

    if (!isLogin) { flag = true; }
    // 检查用户名是否确实存在（只有在布隆过滤器说可能存在时才执行）
    snprintf(order, 256, "SELECT password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    if (mysql_query(sql, order)) {
        LOG_ERROR("MySQL query error: %s", mysql_error(sql));
        return false;  // 如果查询失败，直接返回false
    }
    res = mysql_store_result(sql);
    if (!res) {
        LOG_ERROR("Failed to retrieve SQL result.");
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    if (row) {  // 用户名在数据库中存在
        string dbPassword = row[0];
        if (isLogin) {
            flag = (pwd == dbPassword);  // 登录情况下，密码也必须匹配
            if (!flag) {
                LOG_DEBUG("Password mismatch.");
            }
        } else {
            flag = false;  // 注册情况下，如果用户已存在，返回失败
            LOG_DEBUG("Username already used.");
        }
    } else {  // 用户名不在数据库中
        if (!isLogin) {  // 注册新用户
            snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
            if (mysql_query(sql, order)) {
                LOG_ERROR("Failed to insert new user: %s", mysql_error(sql));
                flag = false;
            } else {
                flag = true;  // 新用户成功注册
                LOG_DEBUG("New user registered.");
            }
        }
    }
    mysql_free_result(res);
    return flag;
}

std::string HttpRequest::path() const {
    return path_;
}

std::string &HttpRequest::path() {
    return path_;
}

std::string HttpRequest::method() const {
    return method_;
}

std::string HttpRequest::version() const {
    return version_;
}

std::string HttpRequest::GetPost(const std::string &key) const {
    assert(key != "");
    if (post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

std::string HttpRequest::GetPost(const char *key) const {
    assert(key != nullptr);
    if (post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}