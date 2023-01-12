#include "db.h"

void TestRAII()
{
    auto db = DB::MakeDB();
    db->Connect("0.0.0.0", "root", "123456", "cdc", 3306);
    auto con = db->GetConnection();
    auto row = con->Query("select * from test");
}

void TestConnection()
{
    auto db = DB::MakeDB();
    {
        db->Connect("0.0.0.0", "root", "123456", "cdc", 3306);
        auto con = db->GetConnection();
        db->Connect("0.0.0.0", "root", "123456", "cdc", 3306);
        auto row = con->Query("select * from test");
        row.reset();
    }
}

int main()
{
    TestRAII();
    TestConnection();
    mysql_library_end();
    return 0;
}
