#include <unistd.h>
#include "db.h"
#include "thread_pool.h"
void TestRAII()
{
    auto db = DB::MakeDB();
    db->Connect("0.0.0.0", "root", "123456", "cdc", 3306);
    auto con = db->GetConnection();
    auto row = con->Query("select * from test");
}

void TestExit()
{
    {
        threadpool::ThreadPool pool(3);
        sleep(1);
    }
    std::cout << "exit" << std::endl;
}

void SleepMs(size_t ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void TestRun()
{
    struct Runner : public threadpool::Task
    {
        void Run() override
        {
            sleep(1);
            std::cout << "hello,world" << std::endl;
        }
    };
    threadpool::ThreadPool pool(3);
    pool.AddTask(std::make_shared<Runner>());
    SleepMs(10);
    std::cout << pool.GetState() << std::endl;
}

int main()
{
    TestExit();
    TestRun();
    return 0;
}
