#include "db.h"

Con::Con()
    : m_con(nullptr)
    , m_connect_timeout(8)
    , m_reconnect(3)
    , m_version(0)
{}

Con::~Con()
{
    if (m_con)
    {
        mysql_close(m_con);
    }
}

bool Con::Connect(const std::string& host, const std::string& name, const std::string& passwd, const std::string& database, int32_t port, size_t version)
{
    m_version = version;
    if (m_con)
    {
        mysql_close(m_con);
    }
    m_con = mysql_init(nullptr);
    mysql_options(m_con, MYSQL_SET_CHARSET_NAME, "utf8");
    mysql_options(m_con, MYSQL_INIT_COMMAND, "SET NAMES utf8");
    mysql_options(m_con, MYSQL_OPT_CONNECT_TIMEOUT, &m_connect_timeout);
    mysql_options(m_con, MYSQL_OPT_RECONNECT, &m_reconnect);
    mysql_options(m_con, MYSQL_OPT_COMPRESS, nullptr);
    if (!mysql_real_connect(m_con, host.c_str(), name.c_str(), passwd.c_str(), database.c_str(), port, nullptr, 0))
    {
        fprintf(stderr, "Failed to connect to database: Error: %s\n", mysql_error(m_con));
        mysql_close(m_con);
        m_con = nullptr;
    }
    return m_con != nullptr;
}

bool Con::IsHealthy() const
{
    if (!m_con)
    {
        return false;
    }

    if (mysql_ping(m_con) != 0)
    {
        fprintf(stderr, "Failed to ping to database: Error: %s\n", mysql_error(m_con));
        return false;
    }
    return true;
}

MYSQL* Con::GetCon()
{
    return m_con;
}

size_t Con::GetVersion()
{
    return m_version;
}

DBCon::DBCon(std::unique_ptr<Con>& con, std::weak_ptr<DB>& db)
    : m_con(std::move(con))
    , m_db(db)
{}

DBCon::~DBCon()
{
    auto dbptr = m_db.lock();
    if (dbptr)
    {
        dbptr->ReleaseConnection(m_con);
    }
}

std::unique_ptr<Res> DBCon::Query(const std::string& sql) const
{
    if (!m_con || m_con->IsHealthy())
    {
        return nullptr;
    }

    if (mysql_real_query(m_con->GetCon(), sql.c_str(), sql.size()) != 0)
    {
        fprintf(stderr, "Query Error : %s, %s\n", mysql_error(m_con->GetCon()), sql.c_str());
        return nullptr;
    }

    MYSQL_RES* res = mysql_store_result(m_con->GetCon());
    return std::unique_ptr<Res>(new Res(res));
}

std::shared_ptr<DB> DB::MakeDB(size_t max)
{
    return std::shared_ptr<DB>(new DB(max));
}

DB::DB(size_t max)
    : m_max(max == 0 ? 1 : max)
{}

DB::~DB() {}

void DB::Connect(const std::string& host, const std::string& name, const std::string& passwd, const std::string& database, int32_t port)
{
    std::lock_guard<std::mutex> lock(m_mut);
    m_version++;
    for (int i = 0; i < m_max; i++)
    {
        std::unique_ptr<Con> con(new Con);
        if (con->Connect(host, name, passwd, database, port, m_version))
        {
            m_con_list.emplace_back(std::move(con));
        }
    }
}

std::unique_ptr<DBCon> DB::GetConnection()
{
    std::weak_ptr<DB> wptr = shared_from_this();
    {
        std::lock_guard<std::mutex> lock(m_mut);
        if (!m_con_list.empty())
        {
            std::unique_ptr<DBCon> con(new DBCon(m_con_list.back(), wptr));
            m_con_list.pop_back();
            return con;
        }
    }

    std::unique_lock<std::mutex> lock(m_mut);
    m_cond.wait(lock, [&] { return !m_con_list.empty(); });
    std::unique_ptr<DBCon> con(new DBCon(m_con_list.back(), wptr));
    m_con_list.pop_back();
    return con;
}

void DB::CheckConnection()
{
    std::lock_guard<std::mutex> lock(m_mut);
    for (auto& con : m_con_list)
    {
        con->IsHealthy();
    }
}

void DB::ReleaseConnection(std::unique_ptr<Con>& con)
{
    std::lock_guard<std::mutex> lock(m_mut);
    if (m_version == con->GetVersion())
    {
        m_con_list.emplace_back(std::move(con));
    }
}
