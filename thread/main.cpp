#include <iostream>
#include <sstream>
#include <fstream>
#include <deque>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
using namespace std;

#include "system.hpp"

class logservice : private module::basic_thread<logservice>
{
    friend class module::basic_thread<logservice>;
    typedef std::pair<std::chrono::system_clock::time_point, std::string> element_type;
    typedef std::deque<element_type> container_type;
protected :
    int entry(module::basic_thread<logservice>* p) override
    {
        while (!need_stop())
        {
            if (empty()) continue;

            element_type element = pop();

            auto local = std::chrono::system_clock::to_time_t(element.first);
            m_file << '['
                << std::put_time(std::localtime(&local), "%F %T")
                << ']' << ' '
                << element.second << std::endl;
        }

        cout << "entry out" << endl;

        return 0;
    }
    void enter()
    {
        auto local = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        m_file << '['
            << std::put_time(std::localtime(&local), "%F %T")
            << "] [START] logservice started." << std::endl;
    }
    void leave()
    {
        auto local = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        m_file << '['
            << std::put_time(std::localtime(&local), "%F %T")
            << "] [END] logservice finished." << std::endl;
    }

public :
    void push(const element_type::second_type& message)
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        m_depot.push_back(std::make_pair(std::chrono::system_clock::now(), message));
    }
    static logservice& reference()
    {
        static logservice instance;
        return instance;
    }

private :
    bool empty() const
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        return m_depot.empty();
    }
    element_type pop()
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        element_type element = m_depot.front();
        m_depot.pop_front();
        return element;
    }

private :
    logservice()
    {
        m_file.open("test.log", std::ios::app);
        start();
    }
    ~logservice()
    {
        stop();
    }
private :
    std::fstream m_file;

    mutable std::mutex m_mutex;
    container_type m_depot;
};

inline void logger(const char* message)
{
    logservice::reference().push(message);
}

int main()
{
    logger("Hello");

    std::thread([&]() {
        int i = 0;
        while (1) {
            std::stringstream ss;
            ss << i << " from first thread" << std::ends;
            logger(ss.str().c_str());
            ++i;

            if (i == 25000) break;
        }
        logger("last of first thread");
    }).detach();

    logger("first thread start");

    std::thread([&]() {
        int i = 0;
        while (1) {
            std::stringstream ss;
            ss << i << " from second thread" << std::ends;
            logger(ss.str().c_str());
            ++i;

            if (i == 20000) break;
        }
        logger("last of second thread");
    }).detach();

    logger("second thread start");

    std::this_thread::sleep_for(std::chrono::seconds(5));

    logger("Bye");
    return 0;
}