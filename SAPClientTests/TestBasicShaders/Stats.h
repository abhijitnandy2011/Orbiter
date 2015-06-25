/*
 * D3D11Client for Orbiter. 2011.
 */
#pragma once

//#define DEBUG
#define _CRT_SECURE_NO_DEPRECATE

/*
 * Performance counter. This was apparently used via a global PCounter variable.
 * We can try the same.
 */
class PerformanceCounter {
public:
	PerformanceCounter();
	~PerformanceCounter();

	void Start();
	void End();
	void End( char *comment );

	char *GetLine();
	void ShowLine();
private:
	__int64		m_Start;
	double m_Freq;
	char m_Line[64];
	char m_String[128];
};
extern PerformanceCounter *PCounter;

/*
class D3D11Stats {
public:
	D3D11Stats();
	~D3D11Stats();

	void Show();

	DWORD
		tcount,		//triangles
		gcount,		//groups
		mcount;		//meshes

private:
};

D3D11Stats *info;*/

