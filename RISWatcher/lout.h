#ifndef LOUT_H
#define LOUT_H

#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <array>
#include <QString>

template<typename Clock, typename Duration>
std::ostream &operator<<(std::ostream &stream,
  const std::chrono::time_point<Clock, Duration> &time_point)
{
  const time_t time = Clock::to_time_t(time_point);
#if __GNUC__ > 4 || \
    ((__GNUC__ == 4) && __GNUC_MINOR__ > 8 && __GNUC_REVISION__ > 1)
  // Maybe the put_time will be implemented later?
  struct tm tm;
  localtime_r(&time, &tm);
  return stream << std::put_time(&tm, "%c"); // Print standard date&time
#else
  char buffer[26];
  ctime_r(&time, buffer);
  buffer[24] = '\0';  // Removes the newline that is added
  return stream << buffer;
#endif
}

std::ostream& operator << (std::ostream& out,const std::string& in);

std::ostream& operator << (std::ostream& out,const QString& str);

class Lout
{
    static auto tm()
    {
        return std::chrono::system_clock::now();
    }
    const std::array<char,4> tickChars = {'|','/','-','\\'};
    static constexpr size_t brWidth=6;
    std::array<char,4>::const_iterator curTick=tickChars.cbegin();
public:    
    template<typename T> std::ostream& operator << (const T& rhs)
    {
        return std::cout << '['<< tm() << "]     " << rhs;
    }
    void anounse(const std::string& msg);
    void anounse(const QString& msg);
    void brackets(const std::string& str);
    void ok();
    void fail();
    void tick();
};

extern Lout lout;

#endif // LOUT_H
