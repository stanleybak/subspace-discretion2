#include "Timers.h"
#include <vector>

struct TimersData
{
    TimersData(Client& c);

    Client& c;

    vector<Timer*> timersList;
    vector<pair<i32, std::function<void()>>> singleTimers;
};

struct Timer
{
    Timer(shared_ptr<TimersData> td, const char* name, i32 repeatMs, std::function<void()> f)
        : td(td), name(name), repeatMs(repeatMs), curDelay(repeatMs), f(f)
    {
        if (repeatMs <= 0)
        {
            td->c.log->LogError("Tried to set timer with repeatMs=%d", repeatMs);
            curDelay = repeatMs = 1000;
        }

        td->timersList.push_back(this);
    }

    ~Timer()
    {
        // erase it from timersList
        for (u32 i = 0; i < td->timersList.size(); ++i)
        {
            if (td->timersList[i] == this)
            {
                td->timersList[i] = td->timersList[td->timersList.size() - 1];
                td->timersList.pop_back();
                break;
            }
        }
    }

    shared_ptr<TimersData> td;
    const char* name;
    i32 repeatMs;
    i32 curDelay;
    std::function<void()> f;
};

TimersData::TimersData(Client& c) : c(c)
{
}

Timers::Timers(Client& c) : Module(c), data(make_shared<TimersData>(c))
{
}

Timers::~Timers()
{
    for (Timer* t : data->timersList)
        c.log->LogError("Timer was not cleaned up correctly: '%s'", t->name);
}

shared_ptr<Timer> Timers::PeriodicTimer(const char* name, i32 mills, std::function<void()> f)
{
    return make_shared<Timer>(data, name, mills, f);
}

void Timers::AdvanceTime(i32 ms)
{
    for (u32 i = 0; i < data->singleTimers.size(); /* increment in loop */)
    {
        data->singleTimers[i].first -= ms;

        if (data->singleTimers[i].first <= 0)
        {
            data->singleTimers[i].second();

            // delete it by copying last and popping
            data->singleTimers[i] = data->singleTimers[data->singleTimers.size() - 1];
            data->singleTimers.pop_back();
        }
        else
            ++i;  // didn't delete, increment
    }

    for (u32 i = 0; i < data->timersList.size(); ++i)
    {
        data->timersList[i]->curDelay -= ms;

        while (i < data->timersList.size() && data->timersList[i]->curDelay <= 0)
        {
            data->timersList[i]->curDelay += data->timersList[i]->repeatMs;
            data->timersList[i]->f();
        }
    }
}

void Timers::SingleTimer(i32 delayMs, std::function<void()> f)
{
    data->singleTimers.push_back(make_pair(delayMs, f));
}
