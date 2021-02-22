#ifndef LOUT_H
#define LOUT_H

#include <chrono>
#include <ctime>
#include <array>
#include <stack>
#include <QString>
#include <QDateTime>
#include <functional>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <iostream>
#include <sstream>

class Lout
{
public:
    class MessageMask
    {
        uint64_t value;
    public:
        static constexpr MessageMask ones()
        {
            return MessageMask(-1);
        }
        explicit constexpr MessageMask(const uint64_t value):value(value){}
        bool operator&(const MessageMask& rhs) const
        {
            return value & rhs.value;
        }
    };
    class PictureElement
    {
        char chr=' ';
        uint8_t color=15;
    public:
        PictureElement(){}
        PictureElement(const char chr, const uint8_t color):chr(chr),color(color){}
        PictureElement(const PictureElement& other):chr(other.chr),color(other.color){}
        PictureElement& operator =(const PictureElement& other)
        {
            chr=other.chr;
            color=other.color;
            return *this;
        }
        PictureElement& operator =(const char& other)
        {
            chr=other;
            return *this;
        }
        bool isEmpty() const
        {
            return chr == ' ';
        }
        uint8_t getColor() const
        {
            return color;
        }
        char getChr() const
        {
            return chr;
        }
    };
    using Picture=std::vector<std::vector<PictureElement>>;
    enum LogLevel
    {
        Info,
        WorkFlow,
        Debug,
        Trace,
        DeepTrace
    };
private:
    struct ProtectedStream
    {
        std::unique_ptr<std::ostream, std::function<void(std::ostream*)>> str;
        std::recursive_mutex mtx;
        bool lastWasBrackets = true;
        explicit ProtectedStream(const bool isFirst):str(
                                                        std::unique_ptr<std::ostream,
                                                        std::function<void(std::ostream*)>
                                                       >
                                                       (
                                                           isFirst
                                                              ? &std::cout
                                                              : new std::stringstream(),
                                                           [](std::ostream* in)
                                                                               {
                                                                                   if(in!=&std::cout)
                                                                                   {
                                                                                       delete in;
                                                                                   }
                                                                               }
                                                       )
                                               )
        {
        }
    };

    ProtectedStream& output;
    std::stack< std::pair<LogLevel,MessageMask> > logLevels;
    constexpr static std::array<char,4> tickChars{'|','/','-','\\'};
    const std::array<std::string, 2> bars;
    std::array<char,4>::const_iterator curTick=tickChars.cbegin();
    std::stack<size_t> lastX;    
    LogLevel outLevel=Info;
    inline static std::mutex globalMtx;        
    bool hasAnounce = false;    
    inline static std::list< ProtectedStream > storedLogs;
    MessageMask outFilterMask = MessageMask::ones();


    static auto tm();
    void nextTick();
    void indent(const size_t cnt, const char inner, const char chr);
    void flood(size_t cnt, const std::string& filler);
    void indentLineStart();
    void noBr();
    void preIndent();
    void printBrackets(const std::string &str, const int color);
    static ProtectedStream& mkOutput()
    {
        std::lock_guard lck(globalMtx);
        storedLogs.emplace_back(
                                    storedLogs.empty()
                               );
        return storedLogs.back();

    }
public:
    Lout& setOutFilterMask(const uint64_t& rhs)
    {
        return setOutFilterMask(MessageMask(rhs));
    }
    Lout& setOutFilterMask(const MessageMask& rhs)
    {
        outFilterMask = rhs;
        return *this;
    }
    static Lout& getInstance()
    {
        static thread_local Lout out;
        return out;
    }
    Lout::LogLevel getOutLevel() const
    {
        return outLevel;
    }
    void newLine();
    void shift(size_t count);
    void resetX();
    size_t getLastX() const;
    size_t getWidth();
    const QLatin1String fmt;
    const size_t width;
    static constexpr size_t brWidth=6;        
    Lout &brackets(const std::string& str, const int color);
    void tick();
    void percent(const size_t cur,const size_t total);
    Lout();    
    bool canMessage() const;    
    void popMsgLevel();
    Lout& setOutLevel(const LogLevel outLevel)
    {
        this->outLevel=outLevel;
        return *this;
    }
    void doAnounce();
    void print(std::string_view in);
    static size_t strlen(const std::string_view &in);
    static int roll(const std::string_view &in, int i, size_t pos);
    static int roll(const std::string_view &in, int i);
    static std::string_view substr(const std::string_view &in, const size_t pos);
    static std::string_view substr(const std::string_view &in, const size_t pos, const size_t count);
    void printW(const std::string& in, const size_t width, const std::string &filler);
    Lout &draw(const Picture &image);    
    template<bool histMode, typename T> void printHist(const T &in)
    {
        constexpr size_t captionWidth = 8;
        constexpr size_t height = 20;

        const size_t screenW = getWidth();
        const ssize_t width = screenW - captionWidth;
        if(width<=0)
        {
            return;
        }

        if(in.empty())
        {
            flood(height * screenW, bars[0]);
            return;
        }

        const auto maxV = std::max_element(in.cbegin(),in.cend(),[] (const typename T::value_type& a,
                                                                     const typename T::value_type& b)
                                                                     {
                                                                         return a.second < b.second;
                                                                     }
                                                                     )->second;

        const auto minV = std::min_element(in.cbegin(),in.cend(),[] (const typename T::value_type& a,
                                                                     const typename T::value_type& b)
                                                                     {
                                                                         return a.second < b.second;
                                                                     }
                                                                     )->second;
        const auto ampV = maxV==minV ? 1 : maxV - minV;

        const size_t M = std::min(height, size_t(ceil( ampV)));

        const size_t len = std::min(in.size(), size_t(width));
        const size_t start = (width - len) / 2;
        const auto m = ampV / static_cast<typename T::mapped_type>(M);

        for(size_t i=height; i; )
        {
            const auto valueH = static_cast<typename T::mapped_type>(i) * m + minV;
            --i;
            const auto valueL = static_cast<typename T::mapped_type>(i) * m + minV;

            if(i & 1)
            {
                flood(captionWidth, " ");
            }
            else
            {
                printW(std::to_string(valueH), captionWidth, " ");
            }

            flood(start, bars[0]);
            auto pos = in.cbegin();
            for(size_t j = 0; j<len;++j,++pos)
            {
                print(bars[ (pos->second <= valueH || histMode) && pos->second >= valueL ]);
            }

            flood(width-len-start, bars[0]);
        }
    }
    friend Lout &Color(Lout& out, const uint8_t);
    friend Lout &noColor(Lout& out);
    friend Lout &flush(Lout& out);
    friend Lout& operator << (Lout& out, const Lout::LogLevel lvl);
    friend Lout& operator << (Lout& out, const MessageMask& rhs);
};

Lout& operator << (Lout& out, const Lout::LogLevel lvl);
Lout& operator << (Lout& out, const std::string& in);
Lout& operator << (Lout& out, const QString& str);
Lout& operator << (Lout& out, Lout& (*func)(Lout&));
Lout& operator << (Lout& out, std::function<Lout& (Lout&)>&&func);
Lout& operator << (Lout& out, const char* rhs);
Lout& operator << (Lout& out, const size_t rhs);
Lout& operator << (Lout& out, const char rhs);
Lout& operator << (Lout& out, const int32_t rhs);
#if _WIN64 || __x86_64__ || __WASM__
Lout& operator << (Lout& out, const uint32_t rhs);
#endif
Lout& operator << (Lout& out, const time_t rhs);
Lout& operator << (Lout& out, const Lout::PictureElement& rhs);
Lout& operator << (Lout& out, const Lout::Picture& rhs);
Lout& operator << (Lout& out, const float& rhs);
Lout& operator << (Lout& out, const std::thread::id& rhs);
Lout& operator << (Lout& out, const Lout::MessageMask& rhs);

Lout &anounce(Lout &ret);
Lout &flush(Lout& out);
Lout &ok(Lout& out);
Lout &fail(Lout& out);
Lout &newLine(Lout& out);
Lout &pop(Lout& out);
Lout &Color(Lout& out, const uint8_t);
Lout &noColor(Lout& out);

inline auto setColor(const uint8_t color)
{
    return std::bind(Color, std::placeholders::_1, color);
}


#define lout Lout::getInstance()

#endif // LOUT_H
