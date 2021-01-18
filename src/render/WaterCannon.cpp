#include "common.h"

#include "WaterCannon.h"
#include "Vector.h"
#include "General.h"
#include "main.h"
#include "Timer.h"
#include "Pools.h"
#include "Ped.h"
#include "AnimManager.h"
#include "Fire.h"
#include "WaterLevel.h"
#include "Camera.h"

#define WATERCANNONVERTS 12
#define WATERCANNONINDEXES 18

RwIm3DVertex WaterCannonVertices[WATERCANNONVERTS];
RwImVertexIndex WaterCannonIndexList[WATERCANNONINDEXES];

CWaterCannon CWaterCannons::aCannons[NUM_WATERCANNONS];

void CWaterCannon::Init(void)
{
	m_nId = 0;
	m_nCur = 0;
	m_nTimeCreated = CTimer::GetTimeInMilliseconds();
	
	for ( int32 i = 0; i < NUM_SEGMENTPOINTS; i++ )
		m_abUsed[i] = false;
	
	WaterCannonIndexList[0] = 0;
	WaterCannonIndexList[1] = 1;
	WaterCannonIndexList[2] = 2;

	WaterCannonIndexList[3] = 1;
	WaterCannonIndexList[4] = 3;
	WaterCannonIndexList[5] = 2;

	WaterCannonIndexList[6] = 4;
	WaterCannonIndexList[7] = 5;
	WaterCannonIndexList[8] = 6;

	WaterCannonIndexList[9]  = 5;
	WaterCannonIndexList[10] = 7;
	WaterCannonIndexList[11] = 6;

	WaterCannonIndexList[12] = 8;
	WaterCannonIndexList[13] = 9;
	WaterCannonIndexList[14] = 10;

	WaterCannonIndexList[15] = 9;
	WaterCannonIndexList[16] = 11;
	WaterCannonIndexList[17] = 10;
}

void CWaterCannon::Update_OncePerFrame(int16 index)
{
	ASSERT(index < NUM_WATERCANNONS);
	
	if (CTimer::GetTimeInMilliseconds() > m_nTimeCreated + WATERCANNON_LIFETIME )
	{
		m_nCur = (m_nCur + 1) % NUM_SEGMENTPOINTS;
		m_abUsed[m_nCur] = false;
	}
	
	for ( int32 i = 0; i < NUM_SEGMENTPOINTS; i++ )
	{
		if ( m_abUsed[i] )
		{
			m_avecVelocity[i].z += -WATERCANNON_GRAVITY * CTimer::GetTimeStep();
			m_avecPos[i]        += m_avecVelocity[i]    * CTimer::GetTimeStep();
		}
	}
	
	int32 extinguishingPoint = CGeneral::GetRandomNumber() & (NUM_SEGMENTPOINTS - 1);
	if ( m_abUsed[extinguishingPoint] )
		gFireManager.ExtinguishPoint(m_avecPos[extinguishingPoint], 3.0f);
	
	if ( ((index + CTimer::GetFrameCounter()) & 3) == 0 )
		PushPeds();
	
	// free if unused
	
	int32 i = 0;
	while ( 1 )
	{
		if ( m_abUsed[i] )
			break;
		
		if ( ++i >= NUM_SEGMENTPOINTS )
		{
			m_nId = 0;
			return;
		}
	}
}

void CWaterCannon::Update_NewInput(CVector *pos, CVector *dir)
{
	ASSERT(pos != NULL);
	ASSERT(dir != NULL);
	
	m_avecPos[m_nCur]      = *pos;
	m_avecVelocity[m_nCur] = *dir;
	m_abUsed[m_nCur]       = true;
}

void CWaterCannon::Render(void)
{
	extern RwRaster *gpFireHoseRaster;

	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      (void *)FALSE);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE,         (void *)TRUE);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE,       (void *)TRUE);
	RwRenderStateSet(rwRENDERSTATESRCBLEND,          (void *)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND,         (void *)rwBLENDINVSRCALPHA);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER,     (void *)gpFireHoseRaster);
	
	static float fStep = 0.0f;

	fStep += CTimer::GetTimeStep() * 0.12f;

	if (fStep >= 1.0f)
	{
		do
			fStep -= 1.0f;
		while (fStep >= 1.0f);
	}

	RwIm3DVertexSetU(&WaterCannonVertices[0], -fStep);
	RwIm3DVertexSetV(&WaterCannonVertices[0], 0.0f);

	RwIm3DVertexSetU(&WaterCannonVertices[1], -fStep);
	RwIm3DVertexSetV(&WaterCannonVertices[1], 1.0f);

	RwIm3DVertexSetU(&WaterCannonVertices[2], 1.0f - fStep);
	RwIm3DVertexSetV(&WaterCannonVertices[2], 0.0f);

	RwIm3DVertexSetU(&WaterCannonVertices[3], 1.0f - fStep);
	RwIm3DVertexSetV(&WaterCannonVertices[3], 1.0f);

	RwIm3DVertexSetU(&WaterCannonVertices[4], -fStep);
	RwIm3DVertexSetV(&WaterCannonVertices[4], 0.0f);

	RwIm3DVertexSetU(&WaterCannonVertices[5], -fStep);
	RwIm3DVertexSetV(&WaterCannonVertices[5], 1.0f);

	RwIm3DVertexSetU(&WaterCannonVertices[6], 1.0f - fStep);
	RwIm3DVertexSetV(&WaterCannonVertices[6], 0.0f);

	RwIm3DVertexSetU(&WaterCannonVertices[7], 1.0f - fStep);
	RwIm3DVertexSetV(&WaterCannonVertices[7], 1.0f);

	RwIm3DVertexSetU(&WaterCannonVertices[8], -fStep);
	RwIm3DVertexSetV(&WaterCannonVertices[8], 0.0f);

	RwIm3DVertexSetU(&WaterCannonVertices[9], -fStep);
	RwIm3DVertexSetV(&WaterCannonVertices[9], 1.0f);

	RwIm3DVertexSetU(&WaterCannonVertices[10], 1.0f - fStep);
	RwIm3DVertexSetV(&WaterCannonVertices[10], 0.0f);

	RwIm3DVertexSetU(&WaterCannonVertices[11], 1.0f - fStep);
	RwIm3DVertexSetV(&WaterCannonVertices[11], 1.0f);
	
	int16 pointA = m_nCur % NUM_SEGMENTPOINTS;
	int16 pointB = pointA - 1;
	if ( pointB < 0 )
		pointB += NUM_SEGMENTPOINTS;

	bool bInit = false;
	CVector norm;
	CVector rand1;
	CVector rand2;
	
	for ( int32 i = 0; i < NUM_SEGMENTPOINTS - 1; i++ )
	{
		if ( m_abUsed[pointA] && m_abUsed[pointB] )
		{
			CVector dist = m_avecPos[pointB] - m_avecPos[pointA];

			if (dist.MagnitudeSqr() < 25.0f)
			{
				if (!bInit)
				{
					CVector cp = CrossProduct(m_avecPos[pointB] - m_avecPos[pointA], TheCamera.GetForward());
					norm = cp * (0.05f / cp.Magnitude());

					rand1 = CVector
					(
						CGeneral::GetRandomNumberInRange(-1.0f, 1.0f),
						CGeneral::GetRandomNumberInRange(-1.0f, 1.0f),
						CGeneral::GetRandomNumberInRange(-1.0f, 1.0f)
					);

					rand1.Normalise();
					rand1 *= 0.05f;

					rand2 = CVector
					(
						CGeneral::GetRandomNumberInRange(-1.0f, 1.0f),
						CGeneral::GetRandomNumberInRange(-1.0f, 1.0f),
						CGeneral::GetRandomNumberInRange(-1.0f, 1.0f)
					);

					rand2.Normalise();
					rand2 *= 0.05f;

					bInit = true;
				}

				uint32 a = (uint32)((1.0f - float(float(i*i) / 256.0f)) * 255.0f);
				uint32 r = 200;
				uint32 g = 255;
				uint32 b = 255;

				RwIm3DVertexSetRGBA(&WaterCannonVertices[0],  r, g, b, a);
				RwIm3DVertexSetRGBA(&WaterCannonVertices[1],  r, g, b, a);
				RwIm3DVertexSetRGBA(&WaterCannonVertices[2],  r, g, b, a);
				RwIm3DVertexSetRGBA(&WaterCannonVertices[3],  r, g, b, a);
				RwIm3DVertexSetRGBA(&WaterCannonVertices[4],  r, g, b, a);
				RwIm3DVertexSetRGBA(&WaterCannonVertices[5],  r, g, b, a);
				RwIm3DVertexSetRGBA(&WaterCannonVertices[6],  r, g, b, a);
				RwIm3DVertexSetRGBA(&WaterCannonVertices[7],  r, g, b, a);
				RwIm3DVertexSetRGBA(&WaterCannonVertices[8],  r, g, b, a);
				RwIm3DVertexSetRGBA(&WaterCannonVertices[9],  r, g, b, a);
				RwIm3DVertexSetRGBA(&WaterCannonVertices[10], r, g, b, a);
				RwIm3DVertexSetRGBA(&WaterCannonVertices[11], r, g, b, a);

				float size = float(float(i*i) / 16.0f + 2.0f);
				CVector v1 = norm * size;
				CVector v2 = rand1 * size;
				CVector v3 = rand2 * size;

				RwIm3DVertexSetPos(&WaterCannonVertices[0], m_avecPos[pointA].x - v1.x,
					m_avecPos[pointA].y - v1.y,
					m_avecPos[pointA].z - v1.z);

				RwIm3DVertexSetPos(&WaterCannonVertices[1], m_avecPos[pointA].x + v1.x,
					m_avecPos[pointA].y + v1.y,
					m_avecPos[pointA].z + v1.z);

				RwIm3DVertexSetPos(&WaterCannonVertices[2], m_avecPos[pointB].x - v1.x,
					m_avecPos[pointB].y - v1.y,
					m_avecPos[pointB].z - v1.z);

				RwIm3DVertexSetPos(&WaterCannonVertices[3], m_avecPos[pointB].x + v1.x,
					m_avecPos[pointB].y + v1.y,
					m_avecPos[pointB].z + v1.z);

				RwIm3DVertexSetPos(&WaterCannonVertices[4], m_avecPos[pointA].x - v2.x,
					m_avecPos[pointA].y - v2.y,
					m_avecPos[pointA].z - v2.z);

				RwIm3DVertexSetPos(&WaterCannonVertices[5], m_avecPos[pointA].x + v2.x,
					m_avecPos[pointA].y + v2.y,
					m_avecPos[pointA].z + v2.z);

				RwIm3DVertexSetPos(&WaterCannonVertices[6], m_avecPos[pointB].x - v2.x,
					m_avecPos[pointB].y - v2.y,
					m_avecPos[pointB].z - v2.z);

				RwIm3DVertexSetPos(&WaterCannonVertices[7], m_avecPos[pointB].x + v2.x,
					m_avecPos[pointB].y + v2.y,
					m_avecPos[pointB].z + v2.z);

				RwIm3DVertexSetPos(&WaterCannonVertices[8], m_avecPos[pointA].x - v3.x,
					m_avecPos[pointA].y - v3.y,
					m_avecPos[pointA].z - v3.z);

				RwIm3DVertexSetPos(&WaterCannonVertices[9], m_avecPos[pointA].x + v3.x,
					m_avecPos[pointA].y + v3.y,
					m_avecPos[pointA].z + v3.z);

				RwIm3DVertexSetPos(&WaterCannonVertices[10], m_avecPos[pointB].x - v3.x,
					m_avecPos[pointB].y - v3.y,
					m_avecPos[pointB].z - v3.z);

				RwIm3DVertexSetPos(&WaterCannonVertices[11], m_avecPos[pointB].x + v3.x,
					m_avecPos[pointB].y + v3.y,
					m_avecPos[pointB].z + v3.z);

				LittleTest();

				if (RwIm3DTransform(WaterCannonVertices, WATERCANNONVERTS, NULL, rwIM3D_VERTEXUV))
				{
					RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, WaterCannonIndexList, WATERCANNONINDEXES);
					RwIm3DEnd();
				}
			}
		}
		
		pointA = pointB--;
		if ( pointB < 0 )
			pointB += NUM_SEGMENTPOINTS;
	}
	
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      (void *)TRUE);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)FALSE);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE,         (void *)FALSE);
}

void CWaterCannon::PushPeds(void)
{
	float minx = 10000.0f;
	float maxx = -10000.0f;
	float miny = 10000.0f;
	float maxy = -10000.0f;
	float minz = 10000.0f;
	float maxz = -10000.0f;
  
	for ( int32 i = 0; i < NUM_SEGMENTPOINTS; i++ )
	{
		if ( m_abUsed[i] )
		{
			minx = Min(minx, m_avecPos[i].x);
			maxx = Max(maxx, m_avecPos[i].x);
			
			miny = Min(miny, m_avecPos[i].y);
			maxy = Max(maxy, m_avecPos[i].y);
			
			minz = Min(minz, m_avecPos[i].z);
			maxz = Max(maxz, m_avecPos[i].z);
		}
	}
	
	for ( int32 i = CPools::GetPedPool()->GetSize() - 1; i >= 0; i--)
	{
		CPed *ped = CPools::GetPedPool()->GetSlot(i);
		if ( ped )
		{
			if (   ped->GetPosition().x > minx && ped->GetPosition().x < maxx
				&& ped->GetPosition().y > miny && ped->GetPosition().y < maxy
				&& ped->GetPosition().z > minz && ped->GetPosition().z < maxz )
			{
				for ( int32 j = 0; j < NUM_SEGMENTPOINTS; j++ )
				{
					if ( m_abUsed[j] )
					{
						CVector dist = m_avecPos[j] - ped->GetPosition();
						
						if ( dist.MagnitudeSqr() < 5.0f )
						{
							int32 localDir = ped->GetLocalDirection(CVector2D(1.0f, 0.0f));
							
							ped->bIsStanding = false;
							
							ped->ApplyMoveForce(0.0f, 0.0f, 2.0f * CTimer::GetTimeStep());
							
							ped->m_vecMoveSpeed.x = (0.6f * m_avecVelocity[j].x + ped->m_vecMoveSpeed.x) * 0.5f;
							ped->m_vecMoveSpeed.y = (0.6f * m_avecVelocity[j].y + ped->m_vecMoveSpeed.y) * 0.5f;
							
							ped->SetFall(2000, AnimationId(ANIM_KO_SKID_FRONT + localDir), 0);
							
							CFire *fire = ped->m_pFire;
							if ( fire )
								fire->Extinguish();
							
							j = NUM_SEGMENTPOINTS;
						}
					}
				}
			}
		}
	}	
}

void CWaterCannons::Init(void)
{
	for ( int32 i = 0; i < NUM_WATERCANNONS; i++ )
		aCannons[i].Init();
}

void CWaterCannons::UpdateOne(uint32 id, CVector *pos, CVector *dir)
{
	ASSERT(pos != NULL);
	ASSERT(dir != NULL);
	
	// find the one by id
	{
		int32 n = 0;
		while ( n < NUM_WATERCANNONS && id != aCannons[n].m_nId )
			n++;
		
		if ( n < NUM_WATERCANNONS )
		{
			aCannons[n].Update_NewInput(pos, dir);
			return;
		}
	}
	
	// if no luck then find a free one
	{
		int32 n = 0;
		while ( n < NUM_WATERCANNONS && 0 != aCannons[n].m_nId )
			n++;
		
		if ( n < NUM_WATERCANNONS )
		{
			aCannons[n].Init();
			aCannons[n].m_nId = id;
			aCannons[n].Update_NewInput(pos, dir);
			return;
		}
	}
}

void CWaterCannons::Update(void)
{
	for ( int32 i = 0; i < NUM_WATERCANNONS; i++ )
	{
		if ( aCannons[i].m_nId != 0 )
			aCannons[i].Update_OncePerFrame(i);
	}
}

void CWaterCannons::Render(void)
{
	for ( int32 i = 0; i < NUM_WATERCANNONS; i++ )
	{
		if ( aCannons[i].m_nId != 0 )
			aCannons[i].Render();
	}
}
