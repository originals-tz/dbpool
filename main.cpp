#include "db.h"
void TestRAII()
{
    auto db = DB::MakeDB();
    db->Connect("0.0.0.0", "root", "123456", "cdc", 3306);
    auto con = db->GetConnection();
    auto row = con->Query("select * from test");
}

int main()
{
    TestRAII();
    mysql_library_end();
    return 0;
}
