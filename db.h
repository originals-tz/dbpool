#ifndef _DB_H_
#define _DB_H_

#include <mysql.h>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <list>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

struct Res
{
    Res(MYSQL_RES* res)
        : m_res(res)
    {}

    ~Res() { mysql_free_result(m_res); }
    MYSQL_RES* m_res = nullptr;
};

class Con
{
public:
    Con();
    ~Con();
    bool Connect(const std::string& host, const std::string& name, const std::string& passwd, const std::string& database, int32_t port, size_t version);
    bool IsHealthy() const;
    MYSQL* GetCon();

private:
    MYSQL* m_con;
    int32_t m_connect_timeout;
    int32_t m_reconnect;
    size_t m_version;
};

class DB;
class DBCon
{
public:
    explicit DBCon(std::unique_ptr<Con>& con, std::weak_ptr<DB>& db);
    ~DBCon();

    std::unique_ptr<Res> Query(const std::string& sql) const;

private:
    std::unique_ptr<Con> m_con;
    std::weak_ptr<DB> m_db;
};

class DB : public std::enable_shared_from_this<DB>
{
    friend class DBCon;
    using Lock = std::lock_guard<std::mutex>;
    using UniqueLock = std::unique_lock<std::mutex>;

public:
    static std::shared_ptr<DB> MakeDB(size_t max = 1);

    ~DB();

    void Connect(const std::string& host, const std::string& name, const std::string& passwd, const std::string& database, int32_t port);

    std::unique_ptr<DBCon> GetConnection();

private:
    explicit DB(size_t max);
    void ReleaseConnection(std::unique_ptr<Con>& con);
    size_t m_max;
    size_t m_version;
    std::mutex m_mut;
    std::condition_variable m_cond;
    std::list<std::unique_ptr<Con>> m_con_list;
};

#endif  // !_DB_H_
