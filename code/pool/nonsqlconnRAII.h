//
// Created by sky on 4/18/24.
//

#ifndef WEBSERVER2_NONSQLCONNRAII_H
#define WEBSERVER2_NONSQLCONNRAII_H

class nosql_connectionRAII{

public:
    nosql_connectionRAII(redisContext **con, nosql_connection_pool *connPool);
    ~nosql_connectionRAII();

private:
    redisContext *conRAII;
    nosql_connection_pool *poolRAII;
};

nosql_connectionRAII::nosql_connectionRAII(redisContext **SQL, nosql_connection_pool *connPool){
    *SQL = connPool->GetConnection();

    conRAII = *SQL;
    poolRAII = connPool;
}

nosql_connectionRAII::~nosql_connectionRAII(){
    poolRAII->ReleaseConnection(conRAII);
}

#endif //WEBSERVER2_NONSQLCONNRAII_H
