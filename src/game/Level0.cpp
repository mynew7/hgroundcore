/*
 * Copyright (C) 2005-2008 MaNGOS <http://www.mangosproject.org/>
 *
 * Copyright (C) 2008 Trinity <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "Player.h"
#include "Opcodes.h"
#include "Chat.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "Language.h"
#include "AccountMgr.h"
#include "SystemConfig.h"
#include "revision.h"
#include "Util.h"
#include "GameEvent.h"

bool ChatHandler::HandleAccountXPToggleCommand(const char* args)
{
    if (uint32 account_id = m_session->GetAccountId())
    {
        if (WorldSession *session = sWorld.FindSession(account_id))
        {
            if (session->IsAccountFlagged(ACC_BLIZZLIKE_RATES))
            {
                session->RemoveAccountFlag(ACC_BLIZZLIKE_RATES);
                PSendSysMessage("Now your rates are serverlike: x2.");
            }
            else
            {
                session->AddAccountFlag(ACC_BLIZZLIKE_RATES);
                PSendSysMessage("Now your rates are blizzlike: x1.");
            }
        }
    }
    else
    {
        PSendSysMessage("Specified account not found.");
        SetSentErrorMessage(true);
        return false;
    }

    return true;
}

bool ChatHandler::HandleAccountBonesHideCommand(const char* args)
{
    if (uint32 account_id = m_session->GetAccountId())
    {
        if (WorldSession *session = sWorld.FindSession(account_id))
        {
            if (session->IsAccountFlagged(ACC_HIDE_BONES))
            {
                session->RemoveAccountFlag(ACC_HIDE_BONES);
                PSendSysMessage("Client will show bones for this account now.");
            }
            else
            {
                session->AddAccountFlag(ACC_HIDE_BONES);
                PSendSysMessage("Client won't show bones for this account now.");
            }
        }
    }
    else
    {
        PSendSysMessage("Specified account not found.");
        SetSentErrorMessage(true);
        return false;
    }

    return true;
}

bool ChatHandler::HandleAccountGuildAnnToggleCommand(const char* args)
{
    if (uint32 account_id = m_session->GetAccountId())
    {
        if (WorldSession *session = sWorld.FindSession(account_id))
        {
            if (session->IsAccountFlagged(ACC_DISABLED_GANN))
            {
                session->RemoveAccountFlag(ACC_DISABLED_GANN);
                PSendSysMessage("Guild announces have been enabled for this account.");
            }
            else
            {
                session->AddAccountFlag(ACC_DISABLED_GANN);
                PSendSysMessage("Guild announces have been disabled for this account.");
            }
        }
    }
    else
    {
        PSendSysMessage("Specified account not found.");
        SetSentErrorMessage(true);
        return false;
    }

    return true;
}

bool ChatHandler::HandleAccountBattleGroundAnnCommand(const char* args)
{
    if (uint32 account_id = m_session->GetAccountId())
    {
        if (WorldSession *session = sWorld.FindSession(account_id))
        {
            if (session->IsAccountFlagged(ACC_DISABLED_BGANN))
            {
                session->RemoveAccountFlag(ACC_DISABLED_BGANN);

                AccountsDatabase.PExecute("UPDATE account SET account_flags = account_flags & '%u' WHERE account_id = '%u'", ~ACC_DISABLED_GANN, account_id);
                PSendSysMessage("BattleGround announces have been enabled for this account.");
            }
            else
            {
                session->AddAccountFlag(ACC_DISABLED_BGANN);

                AccountsDatabase.PExecute("UPDATE account SET account_flags = account_flags | '%u' WHERE account_id = '%u'", ACC_DISABLED_GANN, account_id);
                PSendSysMessage("BattleGround announces have been disabled for this account.");
            }
        }
    }
    else
    {
        PSendSysMessage("Specified account not found.");
        SetSentErrorMessage(true);
        return false;
    }

    return true;
}

bool ChatHandler::HandleHelpCommand(const char* args)
{
    char* cmd = strtok((char*)args, " ");
    if (!cmd)
    {
        ShowHelpForCommand(getCommandTable(), "help");
        ShowHelpForCommand(getCommandTable(), "");
    }
    else
    {
        if (!ShowHelpForCommand(getCommandTable(), cmd))
            SendSysMessage(LANG_NO_HELP_CMD);
    }

    return true;
}

bool ChatHandler::HandleCommandsCommand(const char* args)
{
    ShowHelpForCommand(getCommandTable(), "");
    return true;
}

bool ChatHandler::HandleAccountCommand(const char* /*args*/)
{
    uint64 permissions = m_session->GetPermissions();
    PSendSysMessage(LANG_ACCOUNT_LEVEL, permissions);
    return true;
}

bool ChatHandler::HandleStartCommand(const char* /*args*/)
{
    Player *chr = m_session->GetPlayer();

    if (chr->IsTaxiFlying())
    {
        SendSysMessage(LANG_YOU_IN_FLIGHT);
        SetSentErrorMessage(true);
        return false;
    }

    if (chr->isInCombat())
    {
        SendSysMessage(LANG_YOU_IN_COMBAT);
        SetSentErrorMessage(true);
        return false;
    }

    // cast spell Stuck
    chr->CastSpell(chr,7355,false);
    return true;
}

bool ChatHandler::HandleAccountWeatherCommand(const char* args)
{
    if (!*args)
    {
        SendSysMessage(LANG_USE_BOL);
        return true;
    }

    std::string argstr = (char*)args;
    if (argstr == "on")
        m_session->AddOpcodeDisableFlag(OPC_DISABLE_WEATHER);
    else if (argstr == "off")
        m_session->RemoveOpcodeDisableFlag(OPC_DISABLE_WEATHER);
    else
    {
        SendSysMessage(LANG_USE_BOL);
        return true;
    }

    PSendSysMessage(LANG_SET_WEATHER, argstr.c_str());
    return true;
}

bool ChatHandler::HandleServerInfoCommand(const char* /*args*/)
{
    uint32 activeClientsNum = sWorld.GetActiveSessionCount();
    uint32 queuedClientsNum = sWorld.GetQueuedSessionCount();
    uint32 maxActiveClientsNum = sWorld.GetMaxActiveSessionCount();
    uint32 maxQueuedClientsNum = sWorld.GetMaxQueuedSessionCount();
    std::string str = secsToTimeString(sWorld.GetUptime());
    uint32 updateTime = sWorld.GetUpdateTime();

    PSendSysMessage("Hellground.pl - rev: " _REVISION);
    PSendSysMessage(LANG_CONNECTED_USERS, activeClientsNum, maxActiveClientsNum, queuedClientsNum, maxQueuedClientsNum);
    PSendSysMessage(LANG_UPTIME, str.c_str());
    PSendSysMessage("Update time diff: %u.", updateTime);

    if (sWorld.IsShutdowning())
    {
        PSendSysMessage("");
        PSendSysMessage("Server will %s in: %s", (sWorld.GetShutdownMask() & SHUTDOWN_MASK_RESTART ? "restart" : "be shutteddown"), secsToTimeString(sWorld.GetShutdownTimer()).c_str());
        PSendSysMessage("Reason: %s.", sWorld.GetShutdownReason());
    }

    return true;
}

bool ChatHandler::HandleServerEventsCommand(const char*)
{
    std::string active_events = sGameEventMgr.getActiveEventsString();
    PSendSysMessage(active_events.c_str());//ChatHandler::FillMessageData(&data, this, CHAT_MSG_SYSTEM, LANG_UNIVERSAL, NULL, GetPlayer()->GetGUID(), active_events, NULL);
    return true;
}

bool ChatHandler::HandleDismountCommand(const char* /*args*/)
{
    //If player is not mounted, so go out :)
    if (!m_session->GetPlayer()->IsMounted())
    {
        SendSysMessage(LANG_CHAR_NON_MOUNTED);
        SetSentErrorMessage(true);
        return false;
    }

    if (m_session->GetPlayer()->IsTaxiFlying())
    {
        SendSysMessage(LANG_YOU_IN_FLIGHT);
        SetSentErrorMessage(true);
        return false;
    }

    m_session->GetPlayer()->Unmount();
    m_session->GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
    return true;
}

bool ChatHandler::HandleSaveCommand(const char* /*args*/)
{
    Player *player=m_session->GetPlayer();

    // save GM account without delay and output message (testing, etc)
    if (m_session->GetPermissions() & PERM_GMT)
    {
        player->SaveToDB();
        SendSysMessage(LANG_PLAYER_SAVED);
        return true;
    }

    // save or plan save after 20 sec (logout delay) if current next save time more this value and _not_ output any messages to prevent cheat planning
    uint32 save_interval = sWorld.getConfig(CONFIG_INTERVAL_SAVE);
    if (save_interval==0 || save_interval > 20*1000 && player->GetSaveTimer() <= save_interval - 20*1000)
        player->SaveToDB();

    return true;
}

bool ChatHandler::HandlePasswordCommand(const char* args)
{
    if (!*args)
        return false;

    char *old_pass = strtok ((char*)args, " ");
    char *new_pass = strtok (NULL, " ");
    char *new_pass_c  = strtok (NULL, " ");

    if (!old_pass || !new_pass || !new_pass_c)
        return false;

    std::string password_old = old_pass;
    std::string password_new = new_pass;
    std::string password_new_c = new_pass_c;

    if (strcmp(new_pass, new_pass_c) != 0)
    {
        SendSysMessage (LANG_NEW_PASSWORDS_NOT_MATCH);
        SetSentErrorMessage (true);
        return false;
    }

    if (!AccountMgr::CheckPassword (m_session->GetAccountId(), password_old))
    {
        SendSysMessage (LANG_COMMAND_WRONGOLDPASSWORD);
        SetSentErrorMessage (true);
        return false;
    }

    AccountOpResult result = AccountMgr::ChangePassword(m_session->GetAccountId(), password_new);

    switch (result)
    {
        case AOR_OK:
            SendSysMessage(LANG_COMMAND_PASSWORD);
            break;
        case AOR_PASS_TOO_LONG:
            SendSysMessage(LANG_PASSWORD_TOO_LONG);
            SetSentErrorMessage(true);
            return false;
        case AOR_NAME_NOT_EXIST:                            // not possible case, don't want get account name for output
        default:
            SendSysMessage(LANG_COMMAND_NOTCHANGEPASSWORD);
            SetSentErrorMessage(true);
            return false;
    }

    return true;
}

bool ChatHandler::HandleLockAccountCommand(const char* args)
{
    if (!*args)
    {
        SendSysMessage(LANG_USE_BOL);
        return true;
    }

    std::string argstr = (char*)args;
    if (argstr == "on")
    {
        AccountsDatabase.PExecute("UPDATE account SET account_state_id = '2' WHERE account_id = '%u'",m_session->GetAccountId());
        PSendSysMessage(LANG_COMMAND_ACCLOCKLOCKED);
        return true;
    }

    if (argstr == "off")
    {
        AccountsDatabase.PExecute("UPDATE account SET account_state_id = '1' WHERE account_id = '%u'",m_session->GetAccountId());
        PSendSysMessage(LANG_COMMAND_ACCLOCKUNLOCKED);
        return true;
    }

    SendSysMessage(LANG_USE_BOL);
    return true;
}

/// Display the 'Message of the day' for the realm
bool ChatHandler::HandleServerMotdCommand(const char* /*args*/)
{
    PSendSysMessage(LANG_MOTD_CURRENT, sWorld.GetMotd());
    return true;
}

