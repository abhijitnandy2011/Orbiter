/*
 * Impl
 */

// Self
#include "Stats.h"

// Orbiter
#include "..\..\include\OrbiterAPI.h"

PerformanceCounter::PerformanceCounter()
{
}

PerformanceCounter::~PerformanceCounter()
{
}

void PerformanceCounter::Start()
{
    static LARGE_INTEGER out;

    QueryPerformanceCounter( &out );
    m_Start = (__int64)out.QuadPart;

    QueryPerformanceFrequency( &out );
    m_Freq = (double)out.QuadPart;
    m_Freq /= 1e6;
};

void PerformanceCounter::End()
{
    static LARGE_INTEGER out;
    double result;

    QueryPerformanceCounter( &out );
    result = (double)(out.QuadPart - m_Start)/m_Freq;
    sprintf( m_Line, "%lf mcrs", result );
    oapiWriteLog( m_Line );
}

void PerformanceCounter::End( char *comment )
{
    static LARGE_INTEGER out;
    double result;

    QueryPerformanceCounter( &out );
    result = (double)(out.QuadPart - m_Start)/m_Freq;
    sprintf( m_Line, "%lf mcrs", result );
    strcat( m_String, comment );
    strcat( m_String, m_Line );
    oapiWriteLog( m_String );
}

char *PerformanceCounter::GetLine()
{
    return m_String;
}

void PerformanceCounter::ShowLine()
{
    oapiWriteLog( m_String );
}
