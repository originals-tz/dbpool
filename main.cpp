#include "db.h"
//
// void Test(int id, DB& db)
//{
//    db.Query("select * from test");
//    std::cout << id << ": " << "query finish" << std::endl;
//    std::this_thread::sleep_for(std::chrono::seconds(3));
//}
//
// void TestWait()
//{
//    DB db(2);
//    db.Connect("0.0.0.0", "root", "123456", "cdc", 3306);
//    std::thread t1(&Test, 1, std::ref(db));
//    std::thread t2(&Test, 2, std::ref(db));
//    std::thread t3(&Test, 3, std::ref(db));
//    t1.join();
//    t2.join();
//    t3.join();
//}
//
// void TestRelease()
//{
//    DB db;
//    db.Connect("0.0.0.0", "root", "123456", "cdc", 3306);
//    auto row = db.Query("select * from test");
//    db.Connect("0.0.0.0", "root", "123456", "cdc", 3306);
//    row.reset();
//}
//
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
