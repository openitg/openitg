/* Base definitions file for a LightsDriver_Dynamic module.
 * This file contains all the definitions you'll want, or at least
 * need. If not, let me know and we can update the API. -- Vyhd */

/* API overview, version 1.1:
 *
 * int Load():
 *	Called on initialization. This must return 0 on successful init,
 *	or nonzero for failure. Return value will be used to retrieve an
 *	error message, if you have one defined.
 *
 * void Unload():
 *	Called on de-initialization. All library cleanup must be done here.
 *
 * int Update( struct CLightsState *state ):
 *	Called on lights update. CLightsState and its component enums are
 *	defined below, so you can see that on how the structure is handled.
 *	Returns 0 on successful update, or nonzero for failure. If the
 *	returned error code is < 0, the device is presumed lost and the
 *	driver is unloaded.
 *
 * const LightsModuleInfo* GetModuleInfo():
 *	Returns a pointer to a const LightsModuleInfo struct. This is used to
 *	check the API version and make sure that the binary is compatible
 *	as well as retrieve a description for the logs.
 *
 * const char *GetError( int code ):
 *	Returns a pointer to an error message corresponding to the given
 *	code. This is used to retrieve errors obtained during Init() and
 *	Update().
 *
 * If you're still confused, check out an example .so in Docs.
 */


#ifndef LIGHTSDRIVER_MODULE_DEFS_H
#define LIGHTSDRIVER_MODULE_DEFS_H

#define LIGHTS_API_VERSION_MAJOR 1	// incremented whenever the API breaks old versions
#define LIGHTS_API_VERSION_MINOR 1	// incremented whenever new calls are added

/* we need a uint8_t type for CLightsState, but we don't
 * want to require stdint, especially not for Windows. */
#ifndef GLOBAL_H
  #ifdef _MSC_VER
    typedef unsigned __int8 uint8_t;
  #else
    typedef unsigned char uint8_t;
  #endif
#endif

/* forward struct declarations */
struct CLightsState;
struct LightsModuleInfo;

/* typedefs for function casting, placed here for consistency */
typedef int (*LoadFn)();
typedef int (*UnloadFn)();
typedef int (*UpdateFn)(struct CLightsState*);
typedef const struct LightsModuleInfo* (*GetLightsModuleInfoFn)();
typedef const char* (*GetErrorFn)(int);

#if defined(__cplusplus)
extern "C"
{
#endif

/* contains all the info we need to have about a module. this can be extended
 * but any old variables must remain in place to maintain compatibility. */
struct LightsModuleInfo
{
	short mi_api_ver_major;
	short mi_api_ver_minor;
	short mi_lights_ver_major;
	short mi_lights_ver_minor;
	const char *mi_name;
	const char *mi_author;
};

/* Enumeration definitions that you will definitely want
 * for handling lights within the given CLightsState. */

#ifndef GLOBAL_H
enum CabinetLight
{
	LIGHT_MARQUEE_UP_LEFT,
	LIGHT_MARQUEE_UP_RIGHT,
	LIGHT_MARQUEE_LR_LEFT,
	LIGHT_MARQUEE_LR_RIGHT,
	LIGHT_BUTTONS_LEFT,
	LIGHT_BUTTONS_RIGHT,
	LIGHT_BASS_LEFT,
	LIGHT_BASS_RIGHT,
	NUM_CABINET_LIGHTS,
	LIGHT_INVALID
};

enum GameController
{
	GAME_CONTROLLER_1,	// left controller
	GAME_CONTROLLER_2,	// right controller
	MAX_GAME_CONTROLLERS,	// leave this at the end
	GAME_CONTROLLER_INVALID,
};

enum GameButton
{
	GAME_BUTTON_MENULEFT,
	GAME_BUTTON_MENURIGHT,
	GAME_BUTTON_MENUUP,
	GAME_BUTTON_MENUDOWN,
	GAME_BUTTON_START,
	GAME_BUTTON_SELECT,
	GAME_BUTTON_BACK,
	GAME_BUTTON_COIN,
	GAME_BUTTON_OPERATOR,
	GAME_BUTTON_CUSTOM_01,
	GAME_BUTTON_CUSTOM_02,
	GAME_BUTTON_CUSTOM_03,
	GAME_BUTTON_CUSTOM_04,
	GAME_BUTTON_CUSTOM_05,
	GAME_BUTTON_CUSTOM_06,
	GAME_BUTTON_CUSTOM_07,
	GAME_BUTTON_CUSTOM_08,
	GAME_BUTTON_CUSTOM_09,
	GAME_BUTTON_CUSTOM_10,
	GAME_BUTTON_CUSTOM_11,
	GAME_BUTTON_CUSTOM_12,
	GAME_BUTTON_CUSTOM_13,
	GAME_BUTTON_CUSTOM_14,
	GAME_BUTTON_CUSTOM_15,
	GAME_BUTTON_CUSTOM_16,
	GAME_BUTTON_CUSTOM_17,
	GAME_BUTTON_CUSTOM_18,
	GAME_BUTTON_CUSTOM_19,

	MAX_GAME_BUTTONS,
	GAME_BUTTON_INVALID,
};

/* convenience macros for enumeration loops. Remember that this
 * is C, so you'll need to define your variables beforehand. */
#define FOREACH_ENUM(t,var,start,max)		for( var = start; var < max; var = (enum t)(var+1) )

#define FOREACH_CabinetLight( cl )	FOREACH_ENUM( CabinetLight, cl, LIGHT_MARQUEE_UP_LEFT, NUM_CABINET_LIGHTS )

#define FOREACH_GameController( gc )	FOREACH_ENUM( GameController, gc, GAME_CONTROLLER_1, MAX_GAME_CONTROLLERS )
#define FOREACH_GameButton( gb )	FOREACH_ENUM( GameButton, gb, GAME_BUTTON_MENULEFT, MAX_GAME_BUTTONS )
#define FOREACH_GameButton_Custom( gb )	FOREACH_ENUM( GameButton, gb, GAME_BUTTON_CUSTOM_01, MAX_GAME_BUTTONS )

#endif // !GLOBAL_H

/* This is a C-struct of LightsState, since bool isn't a native type and
 * can vary in size between implementations. uint8_t fixes those both. */
struct CLightsState
{
	uint8_t m_bCabinetLights[NUM_CABINET_LIGHTS];
	uint8_t m_bGameButtonLights[MAX_GAME_CONTROLLERS][MAX_GAME_BUTTONS];

	// This isn't actually a light, but if you're paying attention, you know that already.
	uint8_t m_bCoinCounter;
};

#if defined(__cplusplus)
}
#endif // __cplusplus

#endif // LIGHTSDRIVER_MODULE_DEFS_H
