/*
 * Renders the celestial sphere(planetarium stuff and stars). This sphere is assumed to be
 * at a distance of m_fCelSphereRadius = 1e6f for vertex calculations but co-ordinates
 * are stored in 32 bit floats in rendering buffers.
 * from the Sun based global frame center. It does not use any textures and hence should
 * have no reference to texture managers or texture loading code. Textures for the celestial
 * background are loaded/rendered in the CelBackground class.
 *
 * Has no use for depth checks - whats drawn later hides whats drawn before
 * Stars drawn last
 */

#pragma once

// DirectX
#include <d3dx9math.h>

// Local
#include "D3D11Client.h"

#define MAXSTARVERTEX 32768
#define NSEG 64


namespace Rendering {

// Used to read in star data
struct StarRec {
	float lng, lat, mag;
};

struct STARVERTEX {
	D3DXVECTOR3 pos;
	float col;			//16
};

// Render the Celestial Sphere
class CelSphere {
public:
	CelSphere(D3D11Client &gc);
	~CelSphere();

	/* Calls the other render* functions
	 * 1. Draw Planetarium stuff
	 *      -> Draw Ecliptic grid - aligned with the ecliptic
	 *      -> Draw Ecliptic equator - which is the great circle for ecliptic system
	 *                                 its aligned with orbital plane of solar system
	 *      -> Rotate WVP matrix by angle of obliquity
	 *      -> Draw celestial grid - aligned with Earth's equatorial plane & projected into space
	 *      -> Draw celestial equator
	 *      -> Draw constellations lines - still aligned with celestial grid
	 * 2. Draw stars after resetting WVP matrix
	 */
	void renderCelestialSphere( D3DXMATRIX *VP, float skybrt, const D3DXVECTOR3 *bgcol );

private:
	void initCelSphereEffect();

	// Draw longitudes & latitudes created in createGrid()
	// Used to draw ecliptic grid then tilt the angle by rotating the WVP matrix
	// & draw the ecliptic grid, World matrix is null so only view-projection is needed
	void renderGrid( bool eqline = true );

	// Renders the celestial equator
	void renderGreatCircle();

	// Simple D3D11_PRIMITIVE_TOPOLOGY_LINELIST render of constellations
	void renderConstellations();

	// Renders each buffer in m_vecStarBuffers allocated in loadStars()
	void renderStars( const D3DXVECTOR3 *bgcol );

	// TODO
	void renderCelestialMarkers( D3DXMATRIX *VP );

	/*
	 * For some strange reason we load the star file manually and do
	 * not use the LoadStars() provided by the Orbiter API
	 */
	void loadStars();

	/*
	 * Here we use the Orbiter LoadConstellationLines(),
	 * setup line end points in this function in world space
	 * and put in buffer. They are rendered with depth off anyway
	 * so large world co-ords make no difference.
	 */
	void loadConstellationLines();
	void createGrid();

	/* D3D rendering - these were static before, but we may decide
	 * to have more than 1 celestial sphere later. These are within
	 * rendering loop & used often, so did not use smart pointers
	 * here.
	 */
	// The buffers used by m_VSStarLine for the WVP matrix
	// and by m_PSLine to read the color respectively
    ID3D11Buffer *cb_VS_Star_Line, *cb_PS_Line;
    // The input layout for VS - has pos & color, C for Celestial/Colored Vertex ?
    ID3D11InputLayout *m_ILCVertex;
    // Star and (constellation)line VS - same for both
    ID3D11VertexShader *m_VSStarLine;
    /* m_PSStar for star, gets color from VS with input color * 3
     * m_PSLine for planetarium stuff gets color from cb_PS_Line which is
     * set from plugin - see impl.
     */
    ID3D11PixelShader *m_PSStar, *m_PSLine;


	/* Each pointer in this vector points to an allocated buffer which
	 * must be released in the dtor.
	 */
	std::vector<ID3D11Buffer*> m_vecStarBuffers;
	// No smart ptrs here as we want max speed
	ID3D11Buffer *m_bufConstellationLines, *m_bufLatGrid, *m_bufLngGrid;
	DWORD m_dwNumStarVertices, nmax, m_dwNumConstellationLines;

	// Celestial sphere radius
	double m_fCelSphereRadius;

	// TODO : Not sure what this is - star brightness level ?
	int m_lvlid[256];

	// References to the unique objects used everywhere
	D3D11Client &m_GraphicsClient;
	D3D11Config &m_Config;
	ID3D11Device *m_d3dDevice;
	ID3D11DeviceContext *m_d3dImmContext;
};

} // namespace Rendering
