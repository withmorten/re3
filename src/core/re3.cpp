#include <csignal>
#define WITHWINDOWS
#include "common.h"
#include "crossplatform.h"
#include "Renderer.h"
#include "Credits.h"
#include "Camera.h"
#include "Weather.h"
#include "Clock.h"
#include "World.h"
#include "Vehicle.h"
#include "ModelIndices.h"
#include "Streaming.h"
#include "PathFind.h"
#include "Boat.h"
#include "Heli.h"
#include "Automobile.h"
#include "Console.h"
#include "Debug.h"
#include "Hud.h"
#include "SceneEdit.h"
#include "Pad.h"
#include "PlayerPed.h"
#include "Radar.h"
#include "debugmenu.h"
#include "Frontend.h"
#include "WaterLevel.h"
#include "main.h"
#include "Script.h"
#include "postfx.h"
#include "custompipes.h"
#include "MemoryHeap.h"
#include "FileMgr.h"
#include "PCSave.h"

#ifdef DONT_TRUST_RECOGNIZED_JOYSTICKS
#include "ControllerConfig.h"
#endif

#ifndef _WIN32
#include "assert.h"
#include <stdarg.h>
#endif

#ifdef RWLIBS
extern "C" int vsprintf(char* const _Buffer, char const* const _Format, va_list  _ArgList);
#endif


#ifdef USE_PS2_RAND
unsigned long long myrand_seed = 1;
#else
unsigned long int myrand_seed = 1;
#endif

int
myrand(void)
{
#ifdef USE_PS2_RAND
	// Use our own implementation of rand, stolen from PS2
	myrand_seed = 0x5851F42D4C957F2D * myrand_seed + 1;
	return ((myrand_seed >> 32) & 0x7FFFFFFF);
#else
	// or original codewarrior rand
	myrand_seed = myrand_seed * 1103515245 + 12345;
	return((myrand_seed >> 16) & 0x7FFF);
#endif
}

void
mysrand(unsigned int seed)
{
	myrand_seed = seed;
}

#ifdef CUSTOM_FRONTEND_OPTIONS
#include "frontendoption.h"

void
CustomFrontendOptionsPopulate(void)
{
	// Moved to an array in MenuScreensCustom.cpp, but APIs are still available. see frontendoption.h

	// These work only if we have neo folder, so they're dynamically added
#ifdef EXTENDED_PIPELINES
	const char *vehPipelineNames[] = { "FED_MFX", "FED_NEO" };
	const char *off_on[] = { "FEM_OFF", "FEM_ON" };
	int fd = CFileMgr::OpenFile("neo/neo.txd","r");
	if (fd) {
#ifdef GRAPHICS_MENU_OPTIONS
		FrontendOptionSetCursor(MENUPAGE_GRAPHICS_SETTINGS, -3, false);
		FrontendOptionAddSelect("FED_VPL", vehPipelineNames, ARRAY_SIZE(vehPipelineNames), (int8*)&CustomPipes::VehiclePipeSwitch, false, nil, "VehiclePipeline");
		FrontendOptionAddSelect("FED_PRM", off_on, 2, (int8*)&CustomPipes::RimlightEnable, false, nil, "NeoRimLight");
		FrontendOptionAddSelect("FED_WLM", off_on, 2, (int8*)&CustomPipes::LightmapEnable, false, nil, "NeoLightMaps");
		FrontendOptionAddSelect("FED_RGL", off_on, 2, (int8*)&CustomPipes::GlossEnable, false, nil, "NeoRoadGloss");
#else
		FrontendOptionSetCursor(MENUPAGE_DISPLAY_SETTINGS, -3, false);
		FrontendOptionAddSelect("FED_VPL", vehPipelineNames, ARRAY_SIZE(vehPipelineNames), (int8*)&CustomPipes::VehiclePipeSwitch, false, nil, "VehiclePipeline");
		FrontendOptionAddSelect("FED_PRM", off_on, 2, (int8*)&CustomPipes::RimlightEnable, false, nil, "NeoRimLight");
		FrontendOptionAddSelect("FED_WLM", off_on, 2, (int8*)&CustomPipes::LightmapEnable, false, nil, "NeoLightMaps");
		FrontendOptionAddSelect("FED_RGL", off_on, 2, (int8*)&CustomPipes::GlossEnable, false, nil, "NeoRoadGloss");
#endif
		CFileMgr::CloseFile(fd);
	}
#endif

}
#endif

#ifdef LOAD_INI_SETTINGS
#include "ini_parser.hpp"

linb::ini cfg;
int CheckAndReadIniInt(const char *cat, const char *key, int original)
{
	std::string strval = cfg.get(cat, key, "");
	const char *value = strval.c_str();
	if (value && value[0] != '\0')
		return atoi(value);

	return original;
}

float CheckAndReadIniFloat(const char *cat, const char *key, float original)
{
	std::string strval = cfg.get(cat, key, "");
	const char *value = strval.c_str();
	if (value && value[0] != '\0')
		return atof(value);

	return original;
}

void CheckAndSaveIniInt(const char *cat, const char *key, int val, bool &changed)
{
	char temp[10];
	if (atoi(cfg.get(cat, key, "xxx").c_str()) != val) { // if .ini doesn't have our key, compare with xxx and forcefully add it
		changed = true;
		sprintf(temp, "%u", val);
		cfg.set(cat, key, temp);
	}
}

void CheckAndSaveIniFloat(const char *cat, const char *key, float val, bool &changed)
{
	char temp[10];
	if (atof(cfg.get(cat, key, "xxx").c_str()) != val) { // if .ini doesn't have our key, compare with xxx and forcefully add it
		changed = true;
		sprintf(temp, "%f", val);
		cfg.set(cat, key, temp);
	}
}

void LoadINISettings()
{
	cfg.load_file("re3.ini");

#ifdef DONT_TRUST_RECOGNIZED_JOYSTICKS
	// Written by assuming the codes below will run after _InputInitialiseJoys().
	strcpy(gSelectedJoystickName, cfg.get("DetectJoystick", "JoystickName", "").c_str());
	
	if(gSelectedJoystickName[0] != '\0') {
		for (int i = 0; i <= GLFW_JOYSTICK_LAST; i++) {
			if (glfwJoystickPresent(i) && strncmp(gSelectedJoystickName, glfwGetJoystickName(i), strlen(gSelectedJoystickName)) == 0) {
				if (PSGLOBAL(joy1id) != -1) {
					PSGLOBAL(joy2id) = PSGLOBAL(joy1id);
				}
				PSGLOBAL(joy1id) = i;
				int count;
				glfwGetJoystickButtons(PSGLOBAL(joy1id), &count);
				
				// We need to init and reload bindings, because;
				//	1-joypad button number may differ with saved/prvly connected one
				//	2-bindings are not init'ed if there is no joypad at the start
				ControlsManager.InitDefaultControlConfigJoyPad(count);
				CFileMgr::SetDirMyDocuments();
				int32 gta3set = CFileMgr::OpenFile("gta3.set", "r");
				if (gta3set) {
					ControlsManager.LoadSettings(gta3set);
					CFileMgr::CloseFile(gta3set);
				}
				CFileMgr::SetDir("");
				break;
			}
		}
	}
#endif

#ifdef CUSTOM_FRONTEND_OPTIONS
	for (int i = 0; i < MENUPAGES; i++) {
		for (int j = 0; j < NUM_MENUROWS; j++) {
			CMenuScreenCustom::CMenuEntry &option = aScreens[i].m_aEntries[j];
			if (option.m_Action == MENUACTION_NOTHING)
				break;
				
			// CFO check
			if (option.m_Action < MENUACTION_NOTHING && option.m_CFO->save) {
				// CFO only supports saving uint8 right now
				*option.m_CFO->value = CheckAndReadIniInt("FrontendOptions", option.m_CFO->save, *option.m_CFO->value);
				if (option.m_Action == MENUACTION_CFO_SELECT) {
					option.m_CFOSelect->lastSavedValue = option.m_CFOSelect->displayedValue = *option.m_CFO->value;
				}
			}
		}
	}
#endif

#ifdef EXTENDED_COLOURFILTER
	CPostFX::Intensity = CheckAndReadIniFloat("CustomPipesValues", "PostFXIntensity", CPostFX::Intensity);
#endif
#ifdef EXTENDED_PIPELINES
	CustomPipes::VehicleShininess = CheckAndReadIniFloat("CustomPipesValues", "NeoVehicleShininess", CustomPipes::VehicleShininess);
	CustomPipes::VehicleSpecularity = CheckAndReadIniFloat("CustomPipesValues", "NeoVehicleSpecularity", CustomPipes::VehicleSpecularity);
	CustomPipes::RimlightMult = CheckAndReadIniFloat("CustomPipesValues", "RimlightMult", CustomPipes::RimlightMult);
	CustomPipes::LightmapMult = CheckAndReadIniFloat("CustomPipesValues", "LightmapMult", CustomPipes::LightmapMult);
	CustomPipes::GlossMult = CheckAndReadIniFloat("CustomPipesValues", "GlossMult", CustomPipes::GlossMult);
#endif

#ifdef PROPER_SCALING
	CDraw::ms_bProperScaling = CheckAndReadIniInt("Draw", "ProperScaling", CDraw::ms_bProperScaling);	
#endif
#ifdef FIX_RADAR
	CDraw::ms_bFixRadar      = CheckAndReadIniInt("Draw", "FixRadar", CDraw::ms_bFixRadar);	
#endif
#ifdef FIX_SPRITES
	CDraw::ms_bFixSprites    = CheckAndReadIniInt("Draw", "FixSprites", CDraw::ms_bFixSprites);	
#endif
}

void SaveINISettings()
{
	bool changed = false;

#ifdef DONT_TRUST_RECOGNIZED_JOYSTICKS
	if (strncmp(cfg.get("DetectJoystick", "JoystickName", "").c_str(), gSelectedJoystickName, strlen(gSelectedJoystickName)) != 0) {
		changed = true;
		cfg.set("DetectJoystick", "JoystickName", gSelectedJoystickName);
	}
#endif
#ifdef CUSTOM_FRONTEND_OPTIONS
	for (int i = 0; i < MENUPAGES; i++) {
		for (int j = 0; j < NUM_MENUROWS; j++) {
			CMenuScreenCustom::CMenuEntry &option = aScreens[i].m_aEntries[j];
			if (option.m_Action == MENUACTION_NOTHING)
				break;
				
			if (option.m_Action < MENUACTION_NOTHING && option.m_CFO->save) {
				// Beware: CFO only supports saving uint8 right now
				CheckAndSaveIniInt("FrontendOptions", option.m_CFO->save, *option.m_CFO->value, changed);
			}
		}
	}
#endif

#ifdef EXTENDED_COLOURFILTER
	CheckAndSaveIniFloat("CustomPipesValues", "PostFXIntensity", CPostFX::Intensity, changed);
#endif
#ifdef EXTENDED_PIPELINES
	CheckAndSaveIniFloat("CustomPipesValues", "NeoVehicleShininess", CustomPipes::VehicleShininess, changed);
	CheckAndSaveIniFloat("CustomPipesValues", "NeoVehicleSpecularity", CustomPipes::VehicleSpecularity, changed);
	CheckAndSaveIniFloat("CustomPipesValues", "RimlightMult", CustomPipes::RimlightMult, changed);
	CheckAndSaveIniFloat("CustomPipesValues", "LightmapMult", CustomPipes::LightmapMult, changed);
	CheckAndSaveIniFloat("CustomPipesValues", "GlossMult", CustomPipes::GlossMult, changed);
#endif

#ifdef PROPER_SCALING	
	CheckAndSaveIniInt("Draw", "ProperScaling", CDraw::ms_bProperScaling, changed);	
#endif
#ifdef FIX_RADAR
	CheckAndSaveIniInt("Draw", "FixRadar", CDraw::ms_bFixRadar, changed);
#endif
#ifdef FIX_SPRITES
	CheckAndSaveIniInt("Draw", "FixSprites", CDraw::ms_bFixSprites, changed);	
#endif

	if (changed)
		cfg.write_file("re3.ini");
}

#endif


#ifdef DEBUGMENU
void WeaponCheat();
void HealthCheat();
void TankCheat();
void BlowUpCarsCheat();
void ChangePlayerCheat();
void MayhemCheat();
void EverybodyAttacksPlayerCheat();
void WeaponsForAllCheat();
void FastTimeCheat();
void SlowTimeCheat();
void MoneyCheat();
void ArmourCheat();
void WantedLevelUpCheat();
void WantedLevelDownCheat();
void SunnyWeatherCheat();
void CloudyWeatherCheat();
void RainyWeatherCheat();
void FoggyWeatherCheat();
void FastWeatherCheat();
void OnlyRenderWheelsCheat();
void ChittyChittyBangBangCheat();
void StrongGripCheat();
void NastyLimbsCheat();

DebugMenuEntry *carCol1;
DebugMenuEntry *carCol2;

bool bTrailer = false;

void
SpawnCar(int id)
{
	CVector playerpos;
	CStreaming::RequestModel(id, 0);
	CStreaming::LoadAllRequestedModels(false);
	if(CStreaming::HasModelLoaded(id)){
		playerpos = FindPlayerCoors();
		int node;
		if(CModelInfo::IsBoatModel(id)){
			node = ThePaths.FindNodeClosestToCoors(playerpos, 0, 100.0f, false, false);
			if(node < 0)
				return;
		}

		CVehicle *v;
		if(CModelInfo::IsBoatModel(id))
			v = new CBoat(id, RANDOM_VEHICLE);
		else
			v = new CAutomobile(id, RANDOM_VEHICLE);

		v->bHasBeenOwnedByPlayer = true;
		if(carCol1)
			DebugMenuEntrySetAddress(carCol1, &v->m_currentColour1);
		if(carCol2)
			DebugMenuEntrySetAddress(carCol2, &v->m_currentColour2);

		if(!CModelInfo::IsBoatModel(id))
			v->SetPosition(TheCamera.GetPosition() + TheCamera.GetForward()*15.0f);
		else
			v->SetPosition(ThePaths.m_pathNodes[node].GetPosition());

		v->SetOrientation(0.0f, 0.0f, 3.49f);
		v->SetStatus(STATUS_ABANDONED);
		v->m_nDoorLock = CARLOCK_UNLOCKED;
		CWorld::Add(v);
		((CAutomobile *)v)->PlaceOnRoadProperly();

		if (v && bTrailer && id == MI_LINERUN) {
			CStreaming::RequestModel(MI_YANKEE, 0);
			CStreaming::LoadAllRequestedModels(false);
			if(CStreaming::HasModelLoaded(MI_YANKEE)) {
				CAutomobile *v2;
				v2 = new CAutomobile(MI_YANKEE, RANDOM_VEHICLE);
				v2->SetStatus(STATUS_ABANDONED);
				v2->m_nDoorLock = CARLOCK_UNLOCKED;
				CWorld::Add(v2);
				((CAutomobile *)v)->SetTowLink(v2);
				((CAutomobile *)v2)->PlaceOnRoadProperly();
				bTrailer = false;
			}
		}
	}
}

static void
FixCar(void)
{
	CVehicle *veh = FindPlayerVehicle();
	if(veh == nil)
		return;
	veh->m_fHealth = 1000.0f;
	if(!veh->IsCar())
		return;
	((CAutomobile*)veh)->Damage.SetEngineStatus(0);
	((CAutomobile*)veh)->Fix();
}

#ifdef MENU_MAP
static void
TeleportToWaypoint(void)
{
	if (FindPlayerVehicle()) {
		if (CRadar::TargetMarkerId != -1)
			FindPlayerVehicle()->Teleport(CRadar::TargetMarkerPos + CVector(0.0f, 0.0f, FindPlayerVehicle()->GetColModel()->boundingSphere.center.z));
	} else
		if(CRadar::TargetMarkerId != -1)
			FindPlayerPed()->Teleport(CRadar::TargetMarkerPos + CVector(0.0f, 0.0f, FEET_OFFSET));
}
#endif

static void
SwitchCarCollision(void)
{
	if (FindPlayerVehicle() && FindPlayerVehicle()->IsCar())
		FindPlayerVehicle()->bUsesCollision = !FindPlayerVehicle()->bUsesCollision;
}

static int engineStatus;
static void
SetEngineStatus(void)
{
	CVehicle *veh = FindPlayerVehicle();
	if(veh == nil)
		return;
	if(!veh->IsCar())
		return;
	((CAutomobile*)veh)->Damage.SetEngineStatus(engineStatus);
}

static void
ToggleComedy(void)
{
	CVehicle *veh = FindPlayerVehicle();
	if(veh == nil)
		return;
	veh->bComedyControls = !veh->bComedyControls;
}

static void
PlaceOnRoad(void)
{
	CVehicle *veh = FindPlayerVehicle();
	if(veh == nil)
		return;

	if(veh->IsCar())
		((CAutomobile*)veh)->PlaceOnRoadProperly();
}

static void
ResetCamStatics(void)
{
	TheCamera.Cams[TheCamera.ActiveCam].ResetStatics = true;
}

#ifdef MISSION_SWITCHER
int8 nextMissionToSwitch = 0;
static void
SwitchToMission(void)
{
	CTheScripts::SwitchToMission(nextMissionToSwitch);
}
#endif

static void
SaveGame(int saveSlot)
{
	int8 SaveSlot = PcSaveHelper.SaveSlot(saveSlot);

	if(SaveSlot)
		CHud::SetHelpMessage(TheText.Get("FESZ_L1"), true);
	else
		CHud::SetHelpMessage(TheText.Get("FEC_SVU"), true);
}

#ifdef USE_CUSTOM_ALLOCATOR
static void ParseHeap(void) { gMainHeap.ParseHeap(); }
#endif

static const char *carnames[] = {
	"suv", "riviera", "boxsta", "freight", "nova", "beamer", "humvee", "firetruk", "garbage", "limo", "reliant", "lambo", "carrier", "transit",
	"luton", "testeros", "ambulan", "fbicar", "astrovan", "eldorado", "taxi", "intrepid", "pickup", "icecream", "buggy", "corpse", "police", "swatvan",
	"armour", "viper", "polboat", "bus", "tank", "armytruk", "train", "chopper", "dodo", "coach", "cabbie", "mustang", "ramvan", "rcbug",
	"bellyup", "mrwongs", "fiat", "yardie", "yakuza", "impala", "columb", "hoods", "airtrain", "deaddodo", "speedboat", "fishboat", "panlant", "flatbed",
	"tanker", "escape", "cruiser", "luton2", "ghost",
};

//#include <list>

static CTweakVar** TweakVarsList;
static int TweakVarsListSize = -1;
static bool bAddTweakVarsNow = false;
static const char *pTweakVarsDefaultPath = NULL;

void CTweakVars::Add(CTweakVar *var)
{
	if(TweakVarsListSize == -1) {
		TweakVarsList = (CTweakVar**)malloc(64 * sizeof(CTweakVar*));
		TweakVarsListSize = 0;
	}
	if(TweakVarsListSize > 63)
		TweakVarsList = (CTweakVar**) realloc(TweakVarsList, (TweakVarsListSize + 1) * sizeof(*var));

	TweakVarsList[TweakVarsListSize++] = var;
//	TweakVarsList.push_back(var);
	
	if ( bAddTweakVarsNow )
		var->AddDBG(pTweakVarsDefaultPath);
}

void CTweakVars::AddDBG(const char *path)
{
	pTweakVarsDefaultPath = path;

	for(int i = 0; i < TweakVarsListSize; ++i)
		TweakVarsList[i]->AddDBG(pTweakVarsDefaultPath);
	
	bAddTweakVarsNow = true;
}

void CTweakSwitch::AddDBG(const char *path)
{		
	DebugMenuEntry *e = DebugMenuAddVar(m_pPath == NULL ? path : m_pPath, m_pVarName, (int32_t *)m_pIntVar, m_pFunc, 1, m_nMin, m_nMax, m_aStr);
	DebugMenuEntrySetWrap(e, true);
}
	
void CTweakFunc::AddDBG  (const char *path) { DebugMenuAddCmd     (m_pPath == NULL ? path : m_pPath, m_pVarName, m_pFunc); }
void CTweakBool::AddDBG  (const char *path) { DebugMenuAddVarBool8(m_pPath == NULL ? path : m_pPath, m_pVarName, (int8_t *)m_pBoolVar,  NULL); }
void CTweakInt8::AddDBG  (const char *path) { DebugMenuAddVar     (m_pPath == NULL ? path : m_pPath, m_pVarName, (int8_t *)m_pIntVar,   NULL, m_nStep, m_nLoawerBound, m_nUpperBound, NULL); }
void CTweakUInt8::AddDBG (const char *path) { DebugMenuAddVar     (m_pPath == NULL ? path : m_pPath, m_pVarName, (uint8_t *)m_pIntVar,  NULL, m_nStep, m_nLoawerBound, m_nUpperBound, NULL); }
void CTweakInt16::AddDBG (const char *path) { DebugMenuAddVar     (m_pPath == NULL ? path : m_pPath, m_pVarName, (int16_t *)m_pIntVar,  NULL, m_nStep, m_nLoawerBound, m_nUpperBound, NULL); }
void CTweakUInt16::AddDBG(const char *path) { DebugMenuAddVar     (m_pPath == NULL ? path : m_pPath, m_pVarName, (uint16_t *)m_pIntVar, NULL, m_nStep, m_nLoawerBound, m_nUpperBound, NULL); }
void CTweakInt32::AddDBG (const char *path) { DebugMenuAddVar     (m_pPath == NULL ? path : m_pPath, m_pVarName, (int32_t *)m_pIntVar,  NULL, m_nStep, m_nLoawerBound, m_nUpperBound, NULL); }
void CTweakUInt32::AddDBG(const char *path) { DebugMenuAddVar     (m_pPath == NULL ? path : m_pPath, m_pVarName, (uint32_t *)m_pIntVar, NULL, m_nStep, m_nLoawerBound, m_nUpperBound, NULL); }
void CTweakFloat::AddDBG (const char *path) { DebugMenuAddVar     (m_pPath == NULL ? path : m_pPath, m_pVarName, (float *)m_pIntVar,    NULL, m_nStep, m_nLoawerBound, m_nUpperBound); }

/*
static const char *wt[] = {
			"Sunny", "Cloudy", "Rainy", "Foggy"
		};

SETTWEAKPATH("TEST");		
TWEAKSWITCH(CWeather::NewWeatherType, 0, 3, wt, NULL);
*/

void
DebugMenuPopulate(void)
{
	if(1){
		static const char *weathers[] = {
			"Sunny", "Cloudy", "Rainy", "Foggy"
		};
		DebugMenuEntry *e;
		e = DebugMenuAddVar("Time & Weather", "Current Hour", &CClock::GetHoursRef(), nil, 1, 0, 23, nil);
		DebugMenuEntrySetWrap(e, true);
		e = DebugMenuAddVar("Time & Weather", "Current Minute", &CClock::GetMinutesRef(),
			[](){ CWeather::InterpolationValue = CClock::GetMinutes()/60.0f; }, 1, 0, 59, nil);
			DebugMenuEntrySetWrap(e, true);
		e = DebugMenuAddVar("Time & Weather", "Old Weather", (int16*)&CWeather::OldWeatherType, nil, 1, 0, 3, weathers);
		DebugMenuEntrySetWrap(e, true);
		e = DebugMenuAddVar("Time & Weather", "New Weather", (int16*)&CWeather::NewWeatherType, nil, 1, 0, 3, weathers);
		DebugMenuEntrySetWrap(e, true);
		DebugMenuAddVar("Time & Weather", "Wind", (float*)&CWeather::Wind, nil, 0.1f, 0.0f, 1.0f);
		DebugMenuAddVar("Time & Weather", "Time scale", (float*)&CTimer::GetTimeScale(), nil, 0.1f, 0.0f, 10.0f);

		DebugMenuAddCmd("Cheats", "Weapons", WeaponCheat);
		DebugMenuAddCmd("Cheats", "Money", MoneyCheat);
		DebugMenuAddCmd("Cheats", "Health", HealthCheat);
		DebugMenuAddCmd("Cheats", "Wanted level up", WantedLevelUpCheat);
		DebugMenuAddCmd("Cheats", "Wanted level down", WantedLevelDownCheat);
		DebugMenuAddCmd("Cheats", "Tank", TankCheat);
		DebugMenuAddCmd("Cheats", "Blow up cars", BlowUpCarsCheat);
		DebugMenuAddCmd("Cheats", "Change player", ChangePlayerCheat);
		DebugMenuAddCmd("Cheats", "Mayhem", MayhemCheat);
		DebugMenuAddCmd("Cheats", "Everybody attacks player", EverybodyAttacksPlayerCheat);
		DebugMenuAddCmd("Cheats", "Weapons for all", WeaponsForAllCheat);
		DebugMenuAddCmd("Cheats", "Fast time", FastTimeCheat);
		DebugMenuAddCmd("Cheats", "Slow time", SlowTimeCheat);
		DebugMenuAddCmd("Cheats", "Armour", ArmourCheat);
		DebugMenuAddCmd("Cheats", "Sunny weather", SunnyWeatherCheat);
		DebugMenuAddCmd("Cheats", "Cloudy weather", CloudyWeatherCheat);
		DebugMenuAddCmd("Cheats", "Rainy weather", RainyWeatherCheat);
		DebugMenuAddCmd("Cheats", "Foggy weather", FoggyWeatherCheat);
		DebugMenuAddCmd("Cheats", "Fast weather", FastWeatherCheat);
		DebugMenuAddCmd("Cheats", "Only render wheels", OnlyRenderWheelsCheat);
		DebugMenuAddCmd("Cheats", "Chitty chitty bang bang", ChittyChittyBangBangCheat);
		DebugMenuAddCmd("Cheats", "Strong grip", StrongGripCheat);
		DebugMenuAddCmd("Cheats", "Nasty limbs", NastyLimbsCheat);

		static int spawnCarId = MI_LANDSTAL;
		e = DebugMenuAddVar("Spawn", "Spawn Car ID", &spawnCarId, nil, 1, MI_LANDSTAL, MI_GHOST, carnames);
		DebugMenuEntrySetWrap(e, true);
		DebugMenuAddCmd("Spawn", "Spawn Car", [](){
			if(spawnCarId == MI_TRAIN ||
			   spawnCarId == MI_CHOPPER ||
			   spawnCarId == MI_AIRTRAIN ||
			   spawnCarId == MI_DEADDODO ||
			   spawnCarId == MI_ESCAPE)
				return;
			SpawnCar(spawnCarId);
		});
		static uint8 dummy;
		carCol1 = DebugMenuAddVar("Spawn", "First colour", &dummy, nil, 1, 0, 255, nil);
		carCol2 = DebugMenuAddVar("Spawn", "Second colour", &dummy, nil, 1, 0, 255, nil);
		DebugMenuAddCmd("Spawn", "Spawn Shark", [](){ SpawnCar(MI_STINGER); });
		DebugMenuAddCmd("Spawn", "Spawn Dyablo", [](){ SpawnCar(MI_INFERNUS); });
		DebugMenuAddCmd("Spawn", "Spawn Rocket", [](){ SpawnCar(MI_CHEETAH); });
		DebugMenuAddCmd("Spawn", "Spawn Rumbler", [](){ SpawnCar(MI_BANSHEE); });
		DebugMenuAddCmd("Spawn", "Spawn Esperanto", [](){ SpawnCar(MI_ESPERANT); });
		DebugMenuAddCmd("Spawn", "Spawn Stallion", [](){ SpawnCar(MI_STALLION); });
		DebugMenuAddCmd("Spawn", "Spawn Sentinal", [](){ SpawnCar(MI_KURUMA); });
		DebugMenuAddCmd("Spawn", "Spawn Beamer", [](){ SpawnCar(MI_SENTINEL); });
		DebugMenuAddCmd("Spawn", "Spawn Taxi", [](){ SpawnCar(MI_TAXI); });
		DebugMenuAddCmd("Spawn", "Spawn Cop Car", [](){ SpawnCar(MI_POLICE); });
		DebugMenuAddCmd("Spawn", "Spawn SWAT Van", [](){ SpawnCar(MI_ENFORCER); });
		DebugMenuAddCmd("Spawn", "Spawn Dodo", [](){ SpawnCar(MI_DODO); });
		DebugMenuAddCmd("Spawn", "Spawn PHUQ-2", [](){ SpawnCar(MI_RHINO); });
		DebugMenuAddCmd("Spawn", "Spawn Fire Truck", [](){ SpawnCar(MI_FIRETRUCK); });
		DebugMenuAddCmd("Spawn", "Spawn Police Boat", [](){ SpawnCar(MI_PREDATOR); });
		DebugMenuAddCmd("Spawn", "Spawn Semi Cab with Tanker", []() { bTrailer = true; SpawnCar(MI_LINERUN); });

		DebugMenuAddVarBool8("Render", "Draw hud", &CHud::m_Wants_To_Draw_Hud, nil);
		DebugMenuAddVarBool8("Render", "Draw blips", &Wants_To_Draw_Blips, nil);
		DebugMenuAddVarBool8("Render", "PS2 Alpha test Emu", &gPS2alphaTest, nil);
		DebugMenuAddVarBool8("Render", "Frame limiter", &FrontEndMenuManager.m_PrefsFrameLimiter, nil);
		DebugMenuAddVarBool8("Render", "VSynch", &FrontEndMenuManager.m_PrefsVsync, nil);
		DebugMenuAddVar("Render", "Max FPS", &RsGlobal.maxFPS, nil, 1, 1, 1000, nil);
#ifdef NEW_RENDERER
		DebugMenuAddVarBool8("Render", "new renderer", &gbNewRenderer, nil);
extern bool gbRenderRoads;
extern bool gbRenderEverythingBarRoads;
//extern bool gbRenderFadingInUnderwaterEntities;
extern bool gbRenderFadingInEntities;
extern bool gbRenderWater;
extern bool gbRenderBoats;
extern bool gbRenderVehicles;
extern bool gbRenderWorld0;
extern bool gbRenderWorld1;
extern bool gbRenderWorld2;
		DebugMenuAddVarBool8("Render", "gbRenderRoads", &gbRenderRoads, nil);
		DebugMenuAddVarBool8("Render", "gbRenderEverythingBarRoads", &gbRenderEverythingBarRoads, nil);
//		DebugMenuAddVarBool8("Render", "gbRenderFadingInUnderwaterEntities", &gbRenderFadingInUnderwaterEntities, nil);
		DebugMenuAddVarBool8("Render", "gbRenderFadingInEntities", &gbRenderFadingInEntities, nil);
		DebugMenuAddVarBool8("Render", "gbRenderWater", &gbRenderWater, nil);
		DebugMenuAddVarBool8("Render", "gbRenderBoats", &gbRenderBoats, nil);
		DebugMenuAddVarBool8("Render", "gbRenderVehicles", &gbRenderVehicles, nil);
		DebugMenuAddVarBool8("Render", "gbRenderWorld0", &gbRenderWorld0, nil);
		DebugMenuAddVarBool8("Render", "gbRenderWorld1", &gbRenderWorld1, nil);
		DebugMenuAddVarBool8("Render", "gbRenderWorld2", &gbRenderWorld2, nil);
#endif

#ifdef EXTENDED_COLOURFILTER
		static const char *filternames[] = { "None", "Simple", "Normal", "Mobile" };
		e = DebugMenuAddVar("Render", "Colourfilter", &CPostFX::EffectSwitch, nil, 1, CPostFX::POSTFX_OFF, CPostFX::POSTFX_MOBILE, filternames);
		DebugMenuEntrySetWrap(e, true);
		DebugMenuAddVar("Render", "Intensity", &CPostFX::Intensity, nil, 0.05f, 0, 10.0f);
		DebugMenuAddVarBool8("Render", "Motion Blur", &CPostFX::MotionBlurOn, nil);
#endif
#ifdef EXTENDED_PIPELINES
		static const char *vehpipenames[] = { "MatFX", "Neo" };
		e = DebugMenuAddVar("Render", "Vehicle Pipeline", &CustomPipes::VehiclePipeSwitch, nil,
			1, CustomPipes::VEHICLEPIPE_MATFX, CustomPipes::VEHICLEPIPE_NEO, vehpipenames);
		DebugMenuEntrySetWrap(e, true);
		DebugMenuAddVar("Render", "Neo Vehicle Shininess", &CustomPipes::VehicleShininess, nil, 0.1f, 0, 1.0f);
		DebugMenuAddVar("Render", "Neo Vehicle Specularity", &CustomPipes::VehicleSpecularity, nil, 0.1f, 0, 1.0f);
		DebugMenuAddVarBool8("Render", "Neo Ped Rim light enable", &CustomPipes::RimlightEnable, nil);
		DebugMenuAddVar("Render", "Mult", &CustomPipes::RimlightMult, nil, 0.1f, 0, 1.0f);
		DebugMenuAddVarBool8("Render", "Neo World Lightmaps enable", &CustomPipes::LightmapEnable, nil);
		DebugMenuAddVar("Render", "Mult", &CustomPipes::LightmapMult, nil, 0.1f, 0, 1.0f);
		DebugMenuAddVarBool8("Render", "Neo Road Gloss enable", &CustomPipes::GlossEnable, nil);
		DebugMenuAddVar("Render", "Mult", &CustomPipes::GlossMult, nil, 0.1f, 0, 1.0f);
#endif
		DebugMenuAddVarBool8("Render", "Show Ped Paths", &gbShowPedPaths, nil);
		DebugMenuAddVarBool8("Render", "Show Car Paths", &gbShowCarPaths, nil);
		DebugMenuAddVarBool8("Render", "Show Car Path Links", &gbShowCarPathsLinks, nil);
		DebugMenuAddVarBool8("Render", "Show Ped Road Groups", &gbShowPedRoadGroups, nil);
		DebugMenuAddVarBool8("Render", "Show Car Road Groups", &gbShowCarRoadGroups, nil);
		DebugMenuAddVarBool8("Render", "Show Collision Lines", &gbShowCollisionLines, nil);
		DebugMenuAddVarBool8("Render", "Show Collision Polys", &gbShowCollisionPolys, nil);
		DebugMenuAddVarBool8("Render", "Don't render Buildings", &gbDontRenderBuildings, nil);
		DebugMenuAddVarBool8("Render", "Don't render Big Buildings", &gbDontRenderBigBuildings, nil);
		DebugMenuAddVarBool8("Render", "Don't render Peds", &gbDontRenderPeds, nil);
		DebugMenuAddVarBool8("Render", "Don't render Vehicles", &gbDontRenderVehicles, nil);
		DebugMenuAddVarBool8("Render", "Don't render Objects", &gbDontRenderObjects, nil);
		DebugMenuAddVarBool8("Render", "Don't Render Water", &gbDontRenderWater, nil);		

#ifndef FINAL
		DebugMenuAddVarBool8("Debug", "Print Memory Usage", &gbPrintMemoryUsage, nil);
#ifdef USE_CUSTOM_ALLOCATOR
		DebugMenuAddCmd("Debug", "Parse Heap", ParseHeap);
#endif
#endif
		DebugMenuAddVarBool8("Debug", "Show cullzone debug stuff", &gbShowCullZoneDebugStuff, nil);
		DebugMenuAddVarBool8("Debug", "Disable zone cull", &gbDisableZoneCull, nil);

		DebugMenuAddVarBool8("Debug", "pad 1 -> pad 2", &CPad::m_bMapPadOneToPadTwo, nil);
#ifdef GTA_SCENE_EDIT
		DebugMenuAddVarBool8("Debug", "Edit on", &CSceneEdit::m_bEditOn, nil);
#endif
#ifdef MENU_MAP
		DebugMenuAddCmd("Debug", "Teleport to map waypoint", TeleportToWaypoint);
#endif
		DebugMenuAddCmd("Debug", "Switch car collision", SwitchCarCollision);
		DebugMenuAddVar("Debug", "Engine Status", &engineStatus, nil, 1, 0, 226, nil);
		DebugMenuAddCmd("Debug", "Set Engine Status", SetEngineStatus);
		DebugMenuAddCmd("Debug", "Fix Car", FixCar);
		DebugMenuAddCmd("Debug", "Toggle Comedy Controls", ToggleComedy);
		DebugMenuAddCmd("Debug", "Place Car on Road", PlaceOnRoad);

		DebugMenuAddVarBool8("Debug", "Catalina Heli On", &CHeli::CatalinaHeliOn, nil);
		DebugMenuAddCmd("Debug", "Catalina Fly By", CHeli::StartCatalinaFlyBy);
		DebugMenuAddCmd("Debug", "Catalina Take Off", CHeli::CatalinaTakeOff);
		DebugMenuAddCmd("Debug", "Catalina Fly Away", CHeli::MakeCatalinaHeliFlyAway);
		DebugMenuAddVarBool8("Debug", "Script Heli On", &CHeli::ScriptHeliOn, nil);

		DebugMenuAddVarBool8("Debug", "Toggle popping heads on headshot", &CPed::bPopHeadsOnHeadshot, nil);
		DebugMenuAddCmd("Debug", "Start Credits", CCredits::Start);
		DebugMenuAddCmd("Debug", "Stop Credits", CCredits::Stop);

		DebugMenuAddVarBool8("Debug", "Show DebugStuffInRelease", &gbDebugStuffInRelease, nil);
#ifdef TIMEBARS
		DebugMenuAddVarBool8("Debug", "Show Timebars", &gbShowTimebars, nil);
#endif
#ifdef MISSION_SWITCHER
		DebugMenuEntry *missionEntry;
		static const char* missions[] = {
			"Intro Movie", "Hospital Info Scene", "Police Station Info Scene",
			"RC Diablo Destruction", "RC Mafia Massacre", "RC Rumpo Rampage", "RC Casino Calamity",
			"Patriot Playground", "A Ride In The Park", "Gripped!", "Multistorey Mayhem",
			"Paramedic", "Firefighter", "Vigilante", "Taxi Driver",
			"The Crook", "The Thieves", "The Wife", "Her Lover",
			"Give Me Liberty and Luigi's Girls", "Don't Spank My Bitch Up", "Drive Misty For Me", "Pump-Action Pimp", "The Fuzz Ball",
			"Mike Lips Last Lunch", "Farewell 'Chunky' Lee Chong", "Van Heist", "Cipriani's Chauffeur", "Dead Skunk In The Trunk", "The Getaway",
			"Taking Out The Laundry", "The Pick-Up", "Salvatore's Called A Meeting", "Triads And Tribulations", "Blow Fish", "Chaperone", "Cutting The Grass",
			"Bomb Da Base: Act I", "Bomb Da Base: Act II", "Last Requests", "Turismo", "I Scream, You Scream", "Trial By Fire", "Big'N'Veiny", "Sayonara Salvatore",
			"Under Surveillance", "Paparazzi Purge", "Payday For Ray", "Two-Faced Tanner", "Kanbu Bust-Out", "Grand Theft Auto", "Deal Steal", "Shima", "Smack Down",
			"Silence The Sneak", "Arms Shortage", "Evidence Dash", "Gone Fishing", "Plaster Blaster", "Marked Man",
			"Liberator", "Waka-Gashira Wipeout!", "A Drop In The Ocean", "Bling-Bling Scramble", "Uzi Rider", "Gangcar Round-Up", "Kingdom Come",
			"Grand Theft Aero", "Escort Service", "Decoy", "Love's Disappearance", "Bait", "Espresso-2-Go!", "S.A.M.",
			"Uzi Money", "Toyminator", "Rigged To Blow", "Bullion Run", "Rumble", "The Exchange"
		};

		missionEntry = DebugMenuAddVar("Debug", "Select mission", &nextMissionToSwitch, nil, 1, 0, 79, missions);
		DebugMenuEntrySetWrap(missionEntry, true);
		DebugMenuAddCmd("Debug", "Start selected mission ", SwitchToMission);

		static int saveSlotId = 0;
		static const char *slotNames[] = {
		    "SAVE SLOT 1", "SAVE SLOT 2", "SAVE SLOT 3", "SAVE SLOT 4", "SAVE SLOT 5", "SAVE SLOT 6", "SAVE SLOT 7", "SAVE SLOT 8",
		};
		e = DebugMenuAddVar("Debug", "Save Slot Id", &saveSlotId, nil, 1, 0, 7, slotNames);
		DebugMenuEntrySetWrap(e, true);
		DebugMenuAddCmd("Debug", "Save Game on Selected Slot", []() { SaveGame(saveSlotId); });


#endif

		extern bool PrintDebugCode;
		extern int16 DebugCamMode;
		DebugMenuAddVarBool8("Cam", "Print Debug Code", &PrintDebugCode, nil);
		DebugMenuAddVar("Cam", "Cam Mode", &DebugCamMode, nil, 1, 0, CCam::MODE_EDITOR, nil);
		DebugMenuAddCmd("Cam", "Normal", []() { DebugCamMode = 0; });
		DebugMenuAddCmd("Cam", "Follow Ped With Bind", []() { DebugCamMode = CCam::MODE_FOLLOW_PED_WITH_BIND; });
		DebugMenuAddCmd("Cam", "Reaction", []() { DebugCamMode = CCam::MODE_REACTION; });
		DebugMenuAddCmd("Cam", "Chris", []() { DebugCamMode = CCam::MODE_CHRIS; });
		DebugMenuAddCmd("Cam", "Reset Statics", ResetCamStatics);

		CTweakVars::AddDBG("Debug");
	}
}
#endif

const int   re3_buffsize = 1024;
static char re3_buff[re3_buffsize];

void re3_assert(const char *expr, const char *filename, unsigned int lineno, const char *func)
{
#ifdef _WIN32
	int nCode;

	strcpy_s(re3_buff, re3_buffsize, "Assertion failed!" );
	strcat_s(re3_buff, re3_buffsize, "\n" );	
	
	strcat_s(re3_buff, re3_buffsize, "File: ");
	strcat_s(re3_buff, re3_buffsize, filename );
	strcat_s(re3_buff, re3_buffsize, "\n" );	

	strcat_s(re3_buff, re3_buffsize, "Line: " );
	_itoa_s( lineno, re3_buff + strlen(re3_buff), re3_buffsize - strlen(re3_buff), 10 );
	strcat_s(re3_buff, re3_buffsize, "\n");
	
	strcat_s(re3_buff, re3_buffsize, "Function: ");
	strcat_s(re3_buff, re3_buffsize, func );
	strcat_s(re3_buff, re3_buffsize, "\n" );	
	
	strcat_s(re3_buff, re3_buffsize, "Expression: ");
	strcat_s(re3_buff, re3_buffsize, expr);
	strcat_s(re3_buff, re3_buffsize, "\n");

	strcat_s(re3_buff, re3_buffsize, "\n" );
	strcat_s(re3_buff, re3_buffsize, "(Press Retry to debug the application)");


	nCode = ::MessageBoxA(nil, re3_buff, "RE3 Assertion Failed!",
		MB_ABORTRETRYIGNORE|MB_ICONHAND|MB_SETFOREGROUND|MB_TASKMODAL);

	if (nCode == IDABORT)
	{
		raise(SIGABRT);
		_exit(3);
	}

	if (nCode == IDRETRY)
	{
		__debugbreak();
		return;
	}

	if (nCode == IDIGNORE)
		return;

	abort();
#else
	// TODO
	printf("\nRE3 ASSERT FAILED\n\tFile: %s\n\tLine: %d\n\tFunction: %s\n\tExpression: %s\n",filename,lineno,func,expr);
	assert(false);
#endif
}

void re3_debug(const char *format, ...)
{
	va_list va;
	va_start(va, format);
#ifdef _WIN32
	vsprintf_s(re3_buff, re3_buffsize, format, va);
#else
	vsprintf(re3_buff, format, va);
#endif
	va_end(va);

	printf("%s", re3_buff);
	CDebug::DebugAddText(re3_buff);
}

void re3_trace(const char *filename, unsigned int lineno, const char *func, const char *format, ...)
{
	char buff[re3_buffsize *2];
	va_list va;
	va_start(va, format);
#ifdef _WIN32
	vsprintf_s(re3_buff, re3_buffsize, format, va);
	va_end(va);
	
	sprintf_s(buff, re3_buffsize * 2, "[%s.%s:%d]: %s", filename, func, lineno, re3_buff);
#else
	vsprintf(re3_buff, format, va);
	va_end(va);
	
	sprintf(buff, "[%s.%s:%d]: %s", filename, func, lineno, re3_buff);
#endif

	OutputDebugString(buff);
}

void re3_usererror(const char *format, ...)
{
	va_list va;
	va_start(va, format);
#ifdef _WIN32
	vsprintf_s(re3_buff, re3_buffsize, format, va);
	va_end(va);
	
	::MessageBoxA(nil, re3_buff, "RE3 Error!",
		MB_OK|MB_ICONHAND|MB_SETFOREGROUND|MB_TASKMODAL);

	raise(SIGABRT);
	_exit(3);
#else
	vsprintf(re3_buff, format, va);
	printf("\nRE3 Error!\n\t%s\n",re3_buff);
	assert(false);
#endif
}

#ifdef VALIDATE_SAVE_SIZE
int32 _saveBufCount;
#endif
