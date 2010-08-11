/*=============================================================================

FILE: OEMKeyguard.h

SERVICES:  OEM Keyguard

GENERAL DESCRIPTION:

PUBLIC CLASSES AND STATIC FUNCTIONS:

INITIALIZATION AND SEQUENCING REQUIREMENTS:

(c) COPYRIGHT 2002 QUALCOMM Incorporated.
                    All Rights Reserved.

                    QUALCOMM Proprietary
=============================================================================*/


/*=============================================================================

  $Header: //tsg/UI/main/TSGBREWOEM/src/OEMKeyguard.c#6 $
$DateTime: 2002/11/11 11:34:13 $
  $Author: kjmckay $
  $Change: 68606 $
                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who     what, where, why
--------   ---     ---------------------------------------------------------
10/31/02   kjm     FEATURE_CM_MULTIMODE support
09/17/02   mjv     Include OEMConfig.h instead of OEMCFGI.h and oemnvint.h
09/13/02   mjv     Added animated transition when the keyguard message is
                   closed
08/27/02   mjv     OEMKeyguard_IsEnabled() now returns TRUE when the
                   phone is not idle when the keyguard is enabled.
08/09/02   mjv     Don't show the keyguard annunciator while in a call.
08/01/02   mjv     Added an enum for the unlock sequence states.
07/25/02   mjv     Ported to BREW 2.0.
                   While the keyguard is enabled, only make key beeps as the
                   correct unlock code is being entered.
07/??/02   mjv     Initial revision

=============================================================================*/


/*===========================================================================

                    INCLUDE FILES

===========================================================================*/
#include "AEE_OEMEvent.h"
#include "AEE_OEM.h"
#include "CoreApp_priv.h"
#include "Appscommon.h"
#include "OEMKeyguard.h"
#include "AEE_OEMDispatch.h"
#include "AEETAPI.h"
#include "OEMClassIDs.h"
/*===========================================================================

                    DEFINITIONS AND CONSTANTS

===========================================================================*/

////
// Time in milliseconds before the keyguard message will close
#define TIMEOUT_MS_KEYGUARD_TIMER              (1*1000)

#define KEYGUARD_ERR( str, a, b, c )         ERR( str,a,b,c )
/*===========================================================================

                    TYPE DECLARATIONs

===========================================================================*/

typedef enum {
   UNLOCKSTATE_RESET,        // no keys have been pressed
   UNLOCKSTATE_1PRESSED,     // 1 key has been pressed
   UNLOCKSTATE_1AND2PRESSED  // 1 and 2 keys have been pressed

} UnlockStateEnum;

/*===========================================================================

                    CLASS DEFINITIONS

===========================================================================*/


/*===========================================================================

                    STATIC/LOCAL FUNCTION PROTOTYPES

===========================================================================*/

static boolean OEMPriv_IsPhoneIdle(void);
static void    OEMPriv_ResumeBREW(void);
static void    OEMPriv_MessageTimerCB(void *pUser);
static void    OEMPriv_ResetMessageTimer(void);
#ifdef FEATURE_SET_AUTOKEYGUARD
static void    OEMPriv_AutoguardTimerCB(void *pUser);
static void    OEMPriv_ResetAutoguardTimer(void);
#endif
static boolean OEMPriv_KeyguardEventHandler(AEEEvent  evt,
                                            uint16    wParam,
                                            uint32    dwParam);
static void    OEMPriv_DrawKeyguardMessage(boolean unlockkey);
static void    OEMPriv_DrawKeyguardTime(void);
static void    OEMPriv_DrawMessageCB(void *pUnused);
//static void    OEMPriv_OnCallStatusCB(void *pUnused);
static void    OEMKeyguard_Set_Annunciator_Enable(boolean b_state);

/*===========================================================================

                    STATIC/LOCAL DATA

===========================================================================*/

// TRUE while the keyguard message is being displayed
static boolean sbMessageActive = FALSE;

static ICM *spPhone  = NULL;
static IALERT *spAlert  = NULL;
static IShell *sgpShell = NULL;
static IAnnunciator  *sgpIAnn = NULL;
// TRUE while the keyguard is enabled.
static boolean sbKeyguardEnabled = FALSE;

// Callback used to draw the keyguard enabled message on the screen
static AEECallback   sCBDrawMessage;

// Keyguard unlock state.
//
// Used by OEMPriv_KeyguardEventHandler to keep track of the 123
// unlock sequence.
//
static UnlockStateEnum sUnlockState = UNLOCKSTATE_RESET;

static boolean bDrawMessage = TRUE; 

static IBitmap *pDevBmp = NULL;

/*===========================================================================

                    FUNCTION DEFINITIONS

===========================================================================*/


/*=============================================================================
FUNCTION: OEMPriv_IsPhoneIdle

DESCRIPTION:  Query if the phone is idle

PARAMETERS:
   None

RETURN VALUE:
   boolean:  Returns TRUE if the phone is currently online and in the
             idle phone state.

COMMENTS:

SIDE EFFECTS:

SEE ALSO:

=============================================================================*/
static boolean OEMPriv_IsPhoneIdle(void)
{
#if 1
    AEECMPhInfo phoneInfo;
    if(AEECM_IS_ANYCALL_PRESENT(spPhone))
    {
        return FALSE;
    }

    if(AEE_SUCCESS == ICM_GetPhoneInfo(spPhone, &phoneInfo, sizeof(phoneInfo)))
    {
        if(phoneInfo.oprt_mode != AEECM_OPRT_MODE_ONLINE && phoneInfo.oprt_mode != AEECM_OPRT_MODE_LPM)
        {
            return FALSE;
        }
    }
    KEYGUARD_ERR("OEMPriv_IsPhoneIdle",0,0,0);
    return TRUE;
#else
#ifdef FEATURE_CM_MULTIMODE
    cm_mm_call_info_s_type const *callInfo;
#else
    cm_call_info_s_type const *callInfo;
#endif
    cm_ph_info_s_type const   *phoneInfo;

    if (spPhone)
    {
        callInfo = IPHONE_GetCallInfo(spPhone);

        if (callInfo && callInfo->call_state != CM_CALL_STATE_IDLE)
        {
            // Keyguard is never active while the phone is not idle.
            return FALSE;
        }

        phoneInfo = IPHONE_GetPhoneInfo(spPhone);

        if (phoneInfo && phoneInfo->oprt_mode !=
#ifdef FEATURE_CM_MULTIMODE
                        SYS_OPRT_MODE_ONLINE
#else
                        CM_OPRT_MODE_ONLINE
#endif
        )
        {
            // Keyguard is never active while the phone is not online.
            return FALSE;
        }
    }
    KEYGUARD_ERR("OEMPriv_IsPhoneIdle",0,0,0);
    return TRUE;
#endif
}


/*=============================================================================
FUNCTION: OEMPriv_ResumeBREW

DESCRIPTION:  Remove the keyguard event handler and resume normal BREW
              execution

PARAMETERS:
   None

RETURN VALUE:
   None

COMMENTS:

SIDE EFFECTS:

SEE ALSO:

=============================================================================*/
static void OEMPriv_ResumeBREW(void)
{
    sbMessageActive = FALSE;

#ifdef FEATURE_ANIMATE_TRANSITIONS
    OEM_AnimateTransition(animate_dissolve, FALSE, FALSE);
#endif

    if (!AEE_Resume())
    {
        KEYGUARD_ERR("Unable to resume BREW", 0, 0, 0);
    }
    if(NULL != pDevBmp)
    {
        IBITMAP_Release(pDevBmp);
        pDevBmp = NULL;
    }
    bDrawMessage = TRUE;
}


/*=============================================================================
FUNCTION: OEMPriv_MessageTimerCB

DESCRIPTION:  Callback function when the Message Timer expires

PARAMETERS:
   *pUser:  Unused

RETURN VALUE:
   None

COMMENTS:

SIDE EFFECTS:

SEE ALSO:

=============================================================================*/
static void OEMPriv_MessageTimerCB(void *pUser)
{
    //PARAM_NOT_REF(pUser)

    if (sbMessageActive)
    {
        // Keyguard timed out.  Resume BREW
        OEMPriv_ResumeBREW();
    }
    OEMKeyguard_Set_Annunciator_Enable(TRUE);
}

/*=============================================================================
FUNCTION: OEMPriv_ResetMessageTimer

DESCRIPTION:  Reset the Keyguard Enabled timeout

PARAMETERS:
   None

RETURN VALUE:
   None

COMMENTS:

SIDE EFFECTS:

SEE ALSO:

=============================================================================*/
static void OEMPriv_ResetMessageTimer(void)
{
   (void) AEE_SetSysTimer(TIMEOUT_MS_KEYGUARD_TIMER,
                          OEMPriv_MessageTimerCB,
                          NULL);
}

#ifdef FEATURE_SET_AUTOKEYGUARD
/*=============================================================================
FUNCTION: OEMPriv_AutoguardTimerCB

DESCRIPTION:  Callback function when the Autoguard Timer expires

PARAMETERS:
   *pUser:  Unused

RETURN VALUE:
   None

COMMENTS:

SIDE EFFECTS:

SEE ALSO:

=============================================================================*/
static void OEMPriv_AutoguardTimerCB(void *pUser)
{
    //PARAM_NOT_REF(pUser)
    byte   autoguardState;

    if (sbKeyguardEnabled)
    {
        // Nothing to do, the keyguard is already active!
        return;
    }

    //
    // Ensure autoguard is still enabled before actually enabling the keyguard.
    // The user may have disabled it on us.
    //

    if (SUCCESS == OEM_GetConfig(CFGI_AUTOKEYGUARD,
                            &autoguardState,
                            sizeof(autoguardState)))
    {

        if ( (autoguardState != OEMNV_AKG_OFF) && OEMPriv_IsPhoneIdle())
        {
            OEMKeyguard_SetState(TRUE);
            return;
        }
    }
    OEMPriv_ResetAutoguardTimer();
}
/*=============================================================================
FUNCTION: OEMPriv_ResetAutoguardTimer

DESCRIPTION:  Restart the autoguard timer

PARAMETERS:
   None

RETURN VALUE:
   None

COMMENTS:

SIDE EFFECTS:

SEE ALSO:

=============================================================================*/
static void OEMPriv_ResetAutoguardTimer(void)
{
    byte b;
    int  timeMs;

    if (SUCCESS == OEM_GetConfig(CFGI_AUTOKEYGUARD, &b, sizeof(b)))
    {
        timeMs = 0;

        switch (b)
        {
            case OEMNV_AKG_OFF:
                break;

            case OEMNV_AKG_30SEC:
                timeMs = 30 * 1000;
                break;

            case OEMNV_AKG_2MIN:
                timeMs = 2 * 60 * 1000;
                break;

            case OEMNV_AKG_5MIN:
                timeMs = 5 * 60 * 1000;
                break;

            case OEMNV_AKG_ON: // OEMNV_AKG_ON is an invalid value
            default:
                KEYGUARD_ERR("Unknown autokeyguard setting", 0, 0, 0);
                break;
        }

        if (timeMs > 0)
        {
            (void) AEE_SetSysTimer(timeMs, OEMPriv_AutoguardTimerCB,NULL);
        }
    }
}
#endif
/*=============================================================================
FUNCTION: OEMPriv_KeyguardEventHandler

DESCRIPTION:  Keyguard Event Handler

PARAMETERS:
   *pData:
   evt:
   wParam:
   dwParam:

RETURN VALUE:
   boolean:

COMMENTS:

SIDE EFFECTS:

SEE ALSO:

=============================================================================*/
static boolean OEMPriv_KeyguardEventHandler(AEEEvent  evt,
                                            uint16    wParam,
                                            uint32    dwParam)
{
    // The keyguard message better be on the screen!
    ASSERT(sbMessageActive);
    
    switch (evt)
    {
        case EVT_KEY:
            switch ((AVKType)wParam)
            {           
#if 0            
                case AVK_1:
                    if (UNLOCKSTATE_RESET == sUnlockState)
                    {
                        sUnlockState = UNLOCKSTATE_1PRESSED;
                        // Correct key, make a beep...
                        if (spAlert)
                        {
                            IALERT_KeyBeep(spAlert, (AVKType) wParam, TRUE);
                        }
                    }
                    else
                    {
                        sUnlockState = UNLOCKSTATE_RESET;
                    }
                    break;

                case AVK_2:
                    if (UNLOCKSTATE_1PRESSED == sUnlockState)
                    {
                        sUnlockState = UNLOCKSTATE_1AND2PRESSED;
                        // Correct key, make a beep...
                        if (spAlert)
                        {
                            IALERT_KeyBeep(spAlert, (AVKType) wParam, TRUE);
                        }
                    }
                    else
                    {
                        sUnlockState = UNLOCKSTATE_RESET;
                    }
                    break;

                case AVK_3:
                    if (UNLOCKSTATE_1AND2PRESSED == sUnlockState)
                    {
                        sUnlockState = UNLOCKSTATE_RESET;

                        // Correct key, make a beep...
                        if (spAlert)
                        {
                            IALERT_KeyBeep(spAlert, (AVKType) wParam, TRUE);
                        }

                        // Unlock the keyguard
                        OEMKeyguard_SetState(FALSE);

                        OEMPriv_ResumeBREW();
                        return TRUE;

                    }
                    else
                    {
                        sUnlockState = UNLOCKSTATE_RESET;
                    }
                    break;
#endif
                case AVK_SELECT:
                    bDrawMessage = TRUE;
                    if (UNLOCKSTATE_RESET == sUnlockState)
                    {
                        sUnlockState = UNLOCKSTATE_1PRESSED;
                        // Correct key, make a beep...
                        if (spAlert)
                        {                    
                            IALERT_KeyBeep(spAlert, (AVKType) wParam, TRUE);
                        }
                        OEMPriv_DrawKeyguardMessage(TRUE);                            
                    }
                    else
                    {
                        OEMPriv_DrawKeyguardMessage(FALSE);                      
                        sUnlockState = UNLOCKSTATE_RESET;
                    }
                    break;

                case AVK_STAR:
                    if (UNLOCKSTATE_1PRESSED == sUnlockState)
                    {
                        sUnlockState = UNLOCKSTATE_RESET;

                        // Correct key, make a beep...
                        if (spAlert)
                        {
                            IALERT_KeyBeep(spAlert, (AVKType) wParam, TRUE);
                        }

                        // Unlock the keyguard
                        OEMKeyguard_SetState(FALSE);

                        OEMPriv_ResumeBREW();
                        return TRUE;

                    }
                    else
                    {
                        if(!bDrawMessage)
                        {
                            OEMPriv_DrawKeyguardTime();
                        }
                        else
                        {
                            OEMPriv_DrawKeyguardMessage(FALSE);
                        }
                        sUnlockState = UNLOCKSTATE_RESET;
                    }
                    bDrawMessage = !bDrawMessage;
                    break;                    

                default:
                    if(!bDrawMessage)
                    {
                        OEMPriv_DrawKeyguardTime();
                    }
                    else
                    {
                        OEMPriv_DrawKeyguardMessage(FALSE);
                    }
                    sUnlockState = UNLOCKSTATE_RESET;
                    bDrawMessage = !bDrawMessage;
                    break;
            }
            break;

        case EVT_KEY_RELEASE:
            // Ensure the key beep is turned off if long keytones are enabled
            if (spAlert)
            {
                IALERT_KeyBeep(spAlert, (AVKType) wParam, FALSE);
            }
            break;

        default:
            break;
    }


    // When the keyguard is enabled we pretend that all events were handled
    // so the events won't be sent to BREW.
    return TRUE;
}


/*=============================================================================
FUNCTION: OEMPriv_DrawKeyguardMessage

DESCRIPTION:  Draws the keyguard enabled message on the display.

PARAMETERS:
   None

RETURN VALUE:
   None

COMMENTS:

SIDE EFFECTS:

SEE ALSO:

=============================================================================*/
static void OEMPriv_DrawKeyguardMessage(boolean unlockkey)
{
    IDisplay      *pd;
    IStatic       *pStatic;
    KEYGUARD_ERR("OEMPriv_DrawKeyguardMessage %x",sgpShell,0,0);
    (void) ISHELL_CreateInstance(sgpShell,AEECLSID_STATIC,(void**) &pStatic);
    (void) ISHELL_CreateInstance(sgpShell,AEECLSID_DISPLAY,(void**) &pd);

    if (pStatic && pd)
    {
        AEEDeviceInfo devinfo = {0};
        IShell      *pShell = AEE_GetShell();
        
        ISHELL_GetDeviceInfo(pShell, &devinfo);
        if(NULL != pDevBmp)
        {
            IDISPLAY_BitBlt(pd, 0, 0, devinfo.cxScreen, devinfo.cyScreen, 
                            pDevBmp, 0, 0, AEE_RO_COPY);
        }
        else
        {
            IBitmap *pTempBmp = NULL;

            IDISPLAY_GetDeviceBitmap(pd, &pTempBmp);
            if(NULL != pTempBmp)
            {
                IBITMAP_CreateCompatibleBitmap(pTempBmp, &pDevBmp, devinfo.cxScreen, devinfo.cyScreen);
                if(NULL != pDevBmp)
                {
                    IBITMAP_BltIn(pDevBmp, 0, 0, devinfo.cxScreen, devinfo.cyScreen, pTempBmp, 0, 0, AEE_RO_COPY);
                }
                IBITMAP_Release(pTempBmp);
            }
        }
        OEMKeyguard_Set_Annunciator_Enable(FALSE);
#ifdef FEATURE_KEYGUARD
        Appscomm_Draw_Keyguard_Msg(pd,pStatic,unlockkey);
#endif
        ISTATIC_Release(pStatic);
        IDISPLAY_Release(pd);
    }
}


static void    OEMPriv_DrawKeyguardTime(void)
{
    IDisplay      *pd = NULL;
    
    (void) ISHELL_CreateInstance(sgpShell,AEECLSID_DISPLAY,(void**) &pd);
    if(pd)
    {
        Appscommon_Draw_Keyguard_Time(pd);
        IDISPLAY_Release(pd);
    }
}
/*=============================================================================
FUNCTION: OEMPriv_DrawMessageCB

DESCRIPTION:  This callback is associated with the sCBDrawMessage AEE callback.
              It is used to draw the keyguard enabled message on the screen.

PARAMETERS:
   *pUnused: Unused

RETURN VALUE:
   None

COMMENTS:

SIDE EFFECTS:

SEE ALSO:

=============================================================================*/
static void OEMPriv_DrawMessageCB(void *pUnused)
{
    //PARAM_NOT_REF(pUnused)

    // Keyguard should be enabled at this point
    ASSERT(sbKeyguardEnabled);
    KEYGUARD_ERR("OEMPriv_DrawMessageCB %d",sbKeyguardEnabled,0,0);
    // Pretend that the keyguard message is already displayed on the
    // screen by setting sbMessageActive to TRUE.
    //
    // Then send the keyguard handler an event.  The handler will realize
    // that the message isn't actually being displayed (because there is
    // currently an active applet) and display it
    sbMessageActive = TRUE;    
    (void) OEMKeyguard_HandleEvent(EVT_USER, 0);
}



/*=============================================================================
FUNCTION: OEMKeyguard_Init

DESCRIPTION:  Called to initialize the OEM Keyguard

PARAMETERS:
   None

RETURN VALUE:
   None

COMMENTS:

SIDE EFFECTS:

SEE ALSO:

=============================================================================*/
void OEMKeyguard_Init(void *pShell,void *pPhone,void *pAlert,void *ann)
{
    spPhone = (ICM *)pPhone;
    spAlert   = (IALERT*)pAlert;
    sgpShell = (IShell*)pShell;
    sgpIAnn = (IAnnunciator*)ann;
    // Register to be notified on any call state change so we can
    // disable the keyguard annunciator while the phone is
    // not idle.
    //if (spPhone)
    //{
    //    (void) IPHONE_OnCallStatus(spPhone,
    //                    OEMPriv_OnCallStatusCB,
    //                    NULL,
    //                    0,
    //                    OCS_IDLE | OCS_INCOMING |
    //                    OCS_ORIG | OCS_OFFLINE  );
    //}
#ifdef FEATURE_SET_AUTOKEYGUARD
    OEMPriv_ResetAutoguardTimer();
#endif
}


/*=============================================================================
FUNCTION: OEMKeyguard_HandleEvent

DESCRIPTION:  This function should be called before sending a key event
              to BREW.

PARAMETERS:
   evt: AEEEvent
   wParam: 16-bit event parameter (dwParam is assumed to be zero)
   *pAlert: IAlert interface

RETURN VALUE:
   boolean: TRUE if the event was handled by the Keyguard and does not
            need to be forwarded to BREW

COMMENTS:

SIDE EFFECTS:

SEE ALSO:

=============================================================================*/
boolean OEMKeyguard_HandleEvent(AEEEvent  evt,    uint16    wParam)
{
    boolean bRet = FALSE;

    if (OEMKeyguard_IsEnabled() && OEMPriv_IsPhoneIdle())
    {
        boolean bKeyPress = FALSE;
        AEECLSID cls = AEE_Active();        
        KEYGUARD_ERR("OEMKeyguard_HandleEvent %d %x %x",sbMessageActive,evt,wParam);

        //{
        //    db_items_value_type  db_value;
        //    db_value.db_backlight_level = TRUE;
        //    db_put(DB_BACKLIGHT_LEVEL, &db_value);
        //}
        
        if(wParam == AVK_CLR)
        {
            OEMPriv_ResumeBREW();
            return FALSE;            
        }            

        if(cls == AEECLSID_ALARMCLOCK || cls == AEECLSID_SCHEDULEAPP)
        {
            if(wParam == AVK_CLR || wParam == AVK_SELECT)
            {
                OEMPriv_ResumeBREW();
                return FALSE;            
            }            
        }        
        if(wParam== AVK_CLR ||wParam == AVK_END || wParam == AVK_POWER)
        {
            return FALSE;
        }

        if (EVT_KEY_PRESS == evt)
        {
            bKeyPress = TRUE;
        }

        if (sbMessageActive)
        {
            //
            // The keyguard curently thinks that it is active.  However an
            // applet may have started up over the keyguard message so it
            // may actually not be active anymore.
            //
            // If this has happened, pretend like we are inactive so the
            // message will be redrawn...
            //

            // Note that AEE_Active() returns AEECLSID_SHELL when
            // AEE_SetEventHandler() has been used to set a default event
            // handler.  Otherwise it will return zero.
            //
            KEYGUARD_ERR("AEE_Active %x",cls,0,0);
            if ( (cls != AEECLSID_SHELL) && (cls != 0))
            {
                sbMessageActive = FALSE;
                bKeyPress = TRUE;
            }
        }

        if (!sbMessageActive)
        {
            //
            // Only activate the keyguard message on a keypress event.
            //
            // This is to prevent the keyguard message from being displayed
            // immediately after the END key is pressed to terminate a call
            // (the END key release event would trigger the keyguard message
            //  which would be really annoying)
            //
            if (bKeyPress)
            {
                sbMessageActive = TRUE;
                sUnlockState = UNLOCKSTATE_RESET;

                // Suspend the current applet.  This call is synchronous so we
                // can be sure that the current appplet is really suspended
                // after this function returns.
                AEE_Suspend();
            }
        }

        if (sbMessageActive)
        {
            OEMPriv_ResetMessageTimer();

            // Pass the event to the keyguard event handler
            bRet = OEMPriv_KeyguardEventHandler(evt, wParam, 0);
        }

    }
#ifdef FEATURE_SET_AUTOKEYGUARD
    else
    {
        OEMPriv_ResetAutoguardTimer();
    }
#endif
    return bRet;
}

/*=============================================================================
FUNCTION: OEMKeyguard_IsEnabled

DESCRIPTION:  Query if the keyguard is currently enabled.

PARAMETERS:
   None

RETURN VALUE:
   boolean:  TRUE if the keyguard is currently enabled

COMMENTS:

SIDE EFFECTS:

SEE ALSO:

=============================================================================*/
boolean OEMKeyguard_IsEnabled(void)
{
   return sbKeyguardEnabled;
}

/*=============================================================================
FUNCTION: OEMKeyguard_SetState

DESCRIPTION:  Enable or disable the keyguard.

PARAMETERS:
   bEnabled [in]:  TRUE if the keyguard should be enabled.
                   FALSE if the keyguard should be disabled.

RETURN VALUE:
   None

COMMENTS:

SIDE EFFECTS:

SEE ALSO:

=============================================================================*/
void OEMKeyguard_SetState(boolean bEnabled)
{
    sbKeyguardEnabled = bEnabled;
    IANNUNCIATOR_SetField (sgpIAnn, ANNUN_FIELD_LOCKSTATUS, sbKeyguardEnabled ? ANNUN_STATE_LOCKSTATUS_ON : ANNUN_STATE_LOCKSTATUS_OFF);

    if (sbKeyguardEnabled)
    {
        bDrawMessage = TRUE;
        
        // Register the DrawMessage callback so the keyguard enabled message
        // will be drawn on the screen.
        //
        // We don't draw the message right now because we may executing
        // in the context of an applet, or in the middle of closing an
        // applet, or any number of other things.
        //
        CALLBACK_Init(&sCBDrawMessage, OEMPriv_DrawMessageCB, NULL);
#if MIN_BREW_VERSION(3,0)
        AEE_ResumeCallback(&sCBDrawMessage, 0);
#else
        AEE_ResumeCallback(&sCBDrawMessage, AEE_RESUME_CB_SYS);
#endif

    }
    else
    {
        OEMKeyguard_Set_Annunciator_Enable(TRUE);
#ifdef FEATURE_SET_AUTOKEYGUARD
        OEMPriv_ResetAutoguardTimer();
#endif
        if(NULL != pDevBmp)
        {
            IBITMAP_Release(pDevBmp);
            pDevBmp = NULL;
        }
    }
}
/*=============================================================================
FUNCTION: OEMPriv_OnCallStatusCB

DESCRIPTION: IPHONE notifier that is invoked on every call state change.

PARAMETERS:
   *pUnused:  Unused

RETURN VALUE:
   None

COMMENTS:

SIDE EFFECTS:

SEE ALSO:

=============================================================================*/
//static void OEMPriv_OnCallStatusCB(void *pUnused)
//{
    //if (sbKeyguardEnabled)
    //{
    //    IANNUNCIATOR_SetField(sgpIAnn, ANNUN_FIELD_LOCKSTATUS,OEMPriv_IsPhoneIdle() ?  ANNUN_STATE_ON : ANNUN_STATE_OFF);
    //}
//}

static void OEMKeyguard_Set_Annunciator_Enable(boolean b_state)
{
    //KEYGUARD_ERR("Annunciator_Enable %d %d",b_state,sbKeyguardEnabled,0);
    //IANNUNCIATOR_EnableAnnunciatorBar(sgpIAnn,AEECLSID_DISPLAY1,b_state);
    //if(b_state)
    //{
    //    IANNUNCIATOR_SetField (sgpIAnn, ANNUN_FIELD_LOCKSTATUS, sbKeyguardEnabled ? ANNUN_STATE_ON : ANNUN_STATE_OFF);
    //}
}