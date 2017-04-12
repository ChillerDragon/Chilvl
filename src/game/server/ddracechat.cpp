/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#include "gamecontext.h"
#include <engine/shared/config.h>
#include <engine/shared/protocol.h>
#include <engine/server/server.h>
#include <game/server/teams.h>
#include <game/server/gamemodes/DDRace.h>
#include <game/version.h>
#include <game/generated/nethash.cpp>
#include <time.h>          //ChillerDragon
#include <sqlite3/sqlite3.h>

#if defined(CONF_SQL)
#include <game/server/score/sql_score.h>
#endif

bool CheckClientID(int ClientID);

//void CGameContext::ConAfk(IConsole::IResult *pResult, void *pUserData)
//{
//	CGameContext *pSelf = (CGameContext *)pUserData;
//
//	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
//	if (!pPlayer)
//		return;
//
//	CCharacter* pChr = pPlayer->GetCharacter();
//	if (!pChr)
//		return;
//
//
//	if (pPlayer->m_Afk)
//	{
//		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "cmd-Info",
//			"You are already Afk.");
//		return;
//	}
//
//	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Info",
//		"You are now Afk. (away from keyboard)");
//
//	//pPlayer->m_Afk = true;
//	pPlayer->m_LastPlaytime = time_get() - time_freq()*g_Config.m_SvMaxAfkVoteTime;
//}

void CGameContext::ConToggleSpawn(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	if (!pPlayer->m_IsSuperModerator)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission. You are not a SuperModerator.");
		return;
	}

	pPlayer->m_IsSuperModSpawn ^= true;

	if (pPlayer->m_IsSuperModSpawn)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "SuperModSpawn activated");
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "SuperModSpawn deactivated");
	}
}

void CGameContext::ConToggleXpMsg(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;


	pPlayer->m_xpmsg ^= true;
	pSelf->SendBroadcast(" ", pResult->m_ClientID);
}

void CGameContext::ConPolicehelper(IConsole::IResult * pResult, void * pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	char aBuf[128];

	if (pResult->NumArguments() == 0)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "**** POLICEHELPER ****");
		pSelf->SendChatTarget(pResult->m_ClientID, "Police[2] can add and remove some helpers.");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/policehelper add <player>'");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/policehelper remove <player>'");
		pSelf->SendChatTarget(pResult->m_ClientID, "These helpers have some police advantages.");
		pSelf->SendChatTarget(pResult->m_ClientID, "*** Personal Stats ***");
		if (pPlayer->m_PoliceRank > 1)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Police[2]: true");
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Police[2]: false");
		}
		if (pPlayer->m_PoliceHelper)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Policehelper: true");
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Policehelper: false");
		}
		return;
	}


	if (pPlayer->m_PoliceRank < 2)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "You have to be atleast Police[2] to use this command with parameters. Check '/policehelper' for more info.");
		return;
	}
	if (pResult->NumArguments() == 1)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Missing parameter: <player>");
		return;
	}

	int helperID = pSelf->GetCIDByName(pResult->GetString(1));
	if (helperID == -1)
	{
		str_format(aBuf, sizeof(aBuf), "Player '%s' is not online.", pResult->GetString(1));
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		return;
	}

	char aPara[32];
	str_format(aPara, sizeof(aPara), "%s", pResult->GetString(0));
	if (!str_comp_nocase(aPara, "add"))
	{
		if (pSelf->m_apPlayers[helperID])
		{
			if (pSelf->m_apPlayers[helperID]->m_PoliceHelper)
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "This player is already policehelper.");
				return; 
			}

			pSelf->m_apPlayers[helperID]->m_PoliceHelper = true;
			str_format(aBuf, sizeof(aBuf), "'%s' gave you the policehelper rank.", pSelf->Server()->ClientName(pResult->m_ClientID));
			pSelf->SendChatTarget(helperID, aBuf);

			str_format(aBuf, sizeof(aBuf), "'%s' is now an policehelper.", pSelf->Server()->ClientName(helperID));
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		}
	}
	else if (!str_comp_nocase(aPara, "remove"))
	{
		if (pSelf->m_apPlayers[helperID])
		{
			if (!pSelf->m_apPlayers[helperID]->m_PoliceHelper)
			{
			pSelf->SendChatTarget(pResult->m_ClientID, "This player is no policehelper.");
			return;
			}

			pSelf->m_apPlayers[helperID]->m_PoliceHelper = false;
			str_format(aBuf, sizeof(aBuf), "'%s' took your policehelper rank.", pSelf->Server()->ClientName(pResult->m_ClientID));
			pSelf->SendChatTarget(helperID, aBuf);

			str_format(aBuf, sizeof(aBuf), "'%s' is no longer an policehelper.", pSelf->Server()->ClientName(helperID));
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		}
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Unknown parameter. Check '/policehelper' for help.");
	}
}

void CGameContext::ConTaserinfo(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;


	if (pPlayer->m_TaserLevel == 0)
	{
		pPlayer->m_TaserPrice = 50000;
	}
	else if (pPlayer->m_TaserLevel == 1)
	{
		pPlayer->m_TaserPrice = 75000;
	}
	else if (pPlayer->m_TaserLevel == 2)
	{
		pPlayer->m_TaserPrice = 100000;
	}
	else if (pPlayer->m_TaserLevel == 3)
	{
		pPlayer->m_TaserPrice = 150000;
	}
	else if (pPlayer->m_TaserLevel == 4)
	{
		pPlayer->m_TaserPrice = 200000;
	}
	else if (pPlayer->m_TaserLevel == 5)
	{
		pPlayer->m_TaserPrice = 200000;
	}
	else if (pPlayer->m_TaserLevel == 6)
	{
		pPlayer->m_TaserPrice = 200000;
	}
	else
	{
		pPlayer->m_TaserPrice = 0;
	}


	char aBuf[256];


	pSelf->SendChatTarget(pResult->m_ClientID, "~~~ TASER INFO ~~~");
	pSelf->SendChatTarget(pResult->m_ClientID, "Police Ranks 3 or higher are allowed to carry a taser.");
	pSelf->SendChatTarget(pResult->m_ClientID, "Use the taser to fight gangsters.");
	pSelf->SendChatTarget(pResult->m_ClientID, "~~~ YOUR TASER STATS ~~~");
	str_format(aBuf, sizeof(aBuf), "TaserLevel: %d/7", pPlayer->m_TaserLevel);
	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	str_format(aBuf, sizeof(aBuf), "Price for the next level: %d", pPlayer->m_TaserPrice);
	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	str_format(aBuf, sizeof(aBuf), "FreezeTime: 0.%d seconds", pPlayer->m_TaserLevel * 5);
	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	str_format(aBuf, sizeof(aBuf), "FailRate: %d%", 0);
	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
}

void CGameContext::ConOfferInfo(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	char aBuf[256];


	pSelf->SendChatTarget(pResult->m_ClientID, "~~~ OFFER INFO ~~~");
	pSelf->SendChatTarget(pResult->m_ClientID, "Users can accept offers with '/<extra> <accept>'");
	pSelf->SendChatTarget(pResult->m_ClientID, "Moderators can give all players one rainbow offer.");
	pSelf->SendChatTarget(pResult->m_ClientID, "SuperModerators can give all players more rainbow offers and one bloody.");
	//pSelf->SendChatTarget(pResult->m_ClientID, "Admins can give all players much more of everything."); //admins can't do shit lul
	pSelf->SendChatTarget(pResult->m_ClientID, "~~~ YOUR OFFER STATS ~~~");
	str_format(aBuf, sizeof(aBuf), "Rainbow: %d", pPlayer->m_rainbow_offer);
	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	str_format(aBuf, sizeof(aBuf), "Bloody: %d", pPlayer->m_bloody_offer);
	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	str_format(aBuf, sizeof(aBuf), "Trail. %d", pPlayer->m_trail_offer);
	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	str_format(aBuf, sizeof(aBuf), "Atom: %d", pPlayer->m_atom_offer);
	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
}

void CGameContext::ConChangelog(IConsole::IResult * pResult, void * pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;


	//RELEASE NOTES:
	//9.4.2017 RELEASED v.0.0.1

	int page = pResult->GetInteger(0); //no parameter -> 0 -> page 1
	if (!page) { page = 1; }
	int pages = 2;
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "page %d/%d		'/changelog <page>'", page, pages);

	if (page == 1)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"=== Changelog (DDNet++ v.0.0.2) ===");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"+ added '/ascii' command");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"+ added block points check '/points'");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"+ added '/hook <power>' command");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"------------------------");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			aBuf);
	}
	else if (page == 2)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"=== Changelog (DDNet++ v.0.0.1) ===");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"+ added SuperModerator");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"+ added Moderator");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"+ added SuperModerator Spawn");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"+ added '/acc_logout' command");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"+ added '/changepassword <old> <new> <new>' command");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"+ added '/poop <amount> <player>' command");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"+ added '/pay <amount> <player>' command");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"+ added '/policeinfo' command");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"+ added '/bomb <command>' command more info '/bomb help'");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"+ added instagib modes (gdm, idm, gSurvival and iSurvival)");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"* dummys now join automatically on server start");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"* improved the blocker bot");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"------------------------");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			aBuf);
	}
	else
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "changelog",
			"unknow page.");
	}
	
}


void CGameContext::ConMinigameinfo(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Info",
		"Minigame made by ChillerDragon.");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Info",
		"If you buy the game you can play it until disconnect");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Info",
		"Controls & Commands:");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Info",
		"'/start_minigame'      Starts the game...");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Info",
		"'/stop_minigame'      Stops the game...");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Info",
		"'/Minigameleft'           move left.");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Info",
		"'/Minigameright'           move right.");
}

void CGameContext::ConShop(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "room_key     %d money | lvl16 | disconnect", g_Config.m_SvRoomPrice);



	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Shop",
		"***************************");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Shop",
		"          ~ SHOP ~");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Shop",
		"***************************");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Shop",
		"type /buy <itemname>");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Shop",
		"***************************");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Shop",
		"ItemName | Price | Needed Level | OwnTime:");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Shop",
		"rainbow       1 500 money | lvl8 | dead");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Shop",
		"bloody         3 500 money | lvl3 | dead");
	/*pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Shop",
		"atom         3 500 money | lvl3 | dead");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Shop",
		"trail         3 500 money | lvl3 | dead");*/
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Shop",
		"chidraqul     250 money | lvl2 | disconnect");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Shop",
		"shit              5 money | lvl0 | forever");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Shop",
		aBuf);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Shop",
		"police          100 000 money | lvl18 | forever");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Shop",
		"taser          50 000 money | police3 | forever");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Shop",
		"pvp_arena_ticket     150 money | lvl0 | 1 use");
}

void CGameContext::ConPoliceChat(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];

	char aBuf[256 + 24];

	str_format(aBuf, 256 + 24, "[%x][Police]%s: %s",
		pPlayer->m_PoliceRank,
		pSelf->Server()->ClientName(pResult->m_ClientID),
		pResult->GetString(0));
	if (pPlayer->m_PoliceRank > 0)
		pSelf->SendChat(-2, CGameContext::CHAT_ALL, aBuf, pResult->m_ClientID);
	else
		pSelf->Console()->Print(
			IConsole::OUTPUT_LEVEL_STANDARD,
			"Police",
			"You are not police.");
}

void CGameContext::ConCredits(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
		"ChillerDragon's Block mod.");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
		"Created by ChillerDragon, timakro, FruchtiHD, Henritees, SarKro, Pikotee, \\toast & Blue");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
		"Based on DDRaceNetwork.");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
		"DDRaceNetwork is maintained by deen.More infos on ddnet.tw");
}

void CGameContext::ConInfo(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
		"ChillerDragon's Block mod. v.0.0.2 (more infos '/changelog')");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"Based on Teeworlds DDraceNetwork Version: " GAME_VERSION);
//#if defined( GIT_SHORTREV_HASH )
//	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
//			"Git revision hash: " GIT_SHORTREV_HASH);
//#endif
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"Official DDraceNetwork site: ddnet.tw");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"For more Info /cmdlist");
	//pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
	//	    "ChillerDragon's Website: www.chillerdragon.weebly.com");
	//pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
	//		"ChillerDragon's Youtube Channels: ChillerDragon & TeeTower");
}
/*
void CGameContext::ConPlayerinfo(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
		"##################################");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
		"Informations baut a player");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
		"##################################");
}
*/
void CGameContext::ConHelp(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;

	if (pResult->NumArguments() == 0)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "help",
				"/cmdlist will show a list of all chat commands");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "help",
				"/help + any command will show you the help for this command");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "help",
				"Example /help settings will display the help about ");
	}
	else
	{
		const char *pArg = pResult->GetString(0);
		const IConsole::CCommandInfo *pCmdInfo =
				pSelf->Console()->GetCommandInfo(pArg, CFGFLAG_SERVER, false);
		if (pCmdInfo && pCmdInfo->m_pHelp)
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "help",
					pCmdInfo->m_pHelp);
		else
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"help",
					"Command is either unknown or you have given a blank command without any parameters.");
	}
}

void CGameContext::ConSettings(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;

	if (pResult->NumArguments() == 0)
	{
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"setting",
				"to check a server setting say /settings and setting's name, setting names are:");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "setting",
				"teams, cheats, collision, hooking, endlesshooking, me, ");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "setting",
				"hitting, oldlaser, timeout, votes, pause and scores");
	}
	else
	{
		const char *pArg = pResult->GetString(0);
		char aBuf[256];
		float ColTemp;
		float HookTemp;
		pSelf->m_Tuning.Get("player_collision", &ColTemp);
		pSelf->m_Tuning.Get("player_hooking", &HookTemp);
		if (str_comp(pArg, "teams") == 0)
		{
			str_format(
					aBuf,
					sizeof(aBuf),
					"%s %s",
					g_Config.m_SvTeam == 1 ?
							"Teams are available on this server" :
							(g_Config.m_SvTeam == 0 || g_Config.m_SvTeam == 3) ?
									"Teams are not available on this server" :
									"You have to be in a team to play on this server", /*g_Config.m_SvTeamStrict ? "and if you die in a team all of you die" : */
									"and if you die in a team only you die");
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
					aBuf);
		}
		else if (str_comp(pArg, "collision") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					ColTemp ?
							"Players can collide on this server" :
							"Players Can't collide on this server");
		}
		else if (str_comp(pArg, "hooking") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					HookTemp ?
							"Players can hook each other on this server" :
							"Players Can't hook each other on this server");
		}
		else if (str_comp(pArg, "endlesshooking") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvEndlessDrag ?
							"Players can hook time is unlimited" :
							"Players can hook time is limited");
		}
		else if (str_comp(pArg, "hitting") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvHit ?
							"Player's weapons affect each other" :
							"Player's weapons has no affect on each other");
		}
		else if (str_comp(pArg, "oldlaser") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvOldLaser ?
							"Lasers can hit you if you shot them and that they pull you towards the bounce origin (Like DDRace Beta)" :
							"Lasers can't hit you if you shot them, and they pull others towards the shooter");
		}
		else if (str_comp(pArg, "me") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvSlashMe ?
							"Players can use /me commands the famous IRC Command" :
							"Players Can't use the /me command");
		}
		else if (str_comp(pArg, "timeout") == 0)
		{
			str_format(aBuf, sizeof(aBuf),
					"The Server Timeout is currently set to %d seconds",
					g_Config.m_ConnTimeout);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
					aBuf);
		}
		else if (str_comp(pArg, "votes") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvVoteKick ?
							"Players can use Callvote menu tab to kick offenders" :
							"Players Can't use the Callvote menu tab to kick offenders");
			if (g_Config.m_SvVoteKick)
				str_format(
						aBuf,
						sizeof(aBuf),
						"Players are banned for %d second(s) if they get voted off",
						g_Config.m_SvVoteKickBantime);
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvVoteKickBantime ?
							aBuf :
							"Players are just kicked and not banned if they get voted off");
		}
		else if (str_comp(pArg, "pause") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvPauseable ?
							g_Config.m_SvPauseTime ?
									"/pause is available on this server and it pauses your time too" :
									"/pause is available on this server but it doesn't pause your time"
									:"/pause is NOT available on this server");
		}
		else if (str_comp(pArg, "scores") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvHideScore ?
							"Scores are private on this server" :
							"Scores are public on this server");
		}
	}
}

void CGameContext::ConRules(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	bool Printed = false;
	if (g_Config.m_SvDDRaceRules)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				"Be nice.");
		Printed = true;
	}
	if (g_Config.m_SvRulesLine1[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine1);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine2[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine2);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine3[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine3);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine4[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine4);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine5[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine5);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine6[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine6);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine7[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine7);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine8[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine8);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine9[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine9);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine10[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine10);
		Printed = true;
	}
	if (!Printed)
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				"No Rules Defined, Kill em all!!");
}

void CGameContext::ConToggleSpec(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	char aBuf[128];

	if(!g_Config.m_SvPauseable)
	{
		ConTogglePause(pResult, pUserData);
		return;
	}

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (pPlayer->GetCharacter() == 0)
	{
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "spec",
	"You can't spec while you are dead/a spectator.");
	return;
	}
	if (pPlayer->m_Paused == CPlayer::PAUSED_SPEC && g_Config.m_SvPauseable)
	{
		ConTogglePause(pResult, pUserData);
		return;
	}

	if (pPlayer->m_Paused == CPlayer::PAUSED_FORCE)
	{
		str_format(aBuf, sizeof(aBuf), "You are force-specced. %ds left.", pPlayer->m_ForcePauseTime/pSelf->Server()->TickSpeed());
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "spec", aBuf);
		return;
	}

	pPlayer->m_Paused = (pPlayer->m_Paused == CPlayer::PAUSED_PAUSED) ? CPlayer::PAUSED_NONE : CPlayer::PAUSED_PAUSED;
}

void CGameContext::ConTogglePause(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID)) return;
	char aBuf[128];

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if(!pPlayer)
		return;

	if (pPlayer->GetCharacter() == 0)
	{
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pause",
	"You can't pause while you are dead/a spectator.");
	return;
	}

	if(pPlayer->m_Paused == CPlayer::PAUSED_FORCE)
	{
		str_format(aBuf, sizeof(aBuf), "You are force-paused. %ds left.", pPlayer->m_ForcePauseTime/pSelf->Server()->TickSpeed());
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pause", aBuf);
		return;
	}

	pPlayer->m_Paused = (pPlayer->m_Paused == CPlayer::PAUSED_SPEC) ? CPlayer::PAUSED_NONE : CPlayer::PAUSED_SPEC;
}

void CGameContext::ConTeamTop5(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

#if defined(CONF_SQL)
	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		if(pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery + pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;
#endif

	if (g_Config.m_SvHideScore)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "teamtop5",
				"Showing the team top 5 is not allowed on this server.");
		return;
	}

	if (pResult->NumArguments() > 0 && pResult->GetInteger(0) >= 0)
		pSelf->Score()->ShowTeamTop5(pResult, pResult->m_ClientID, pUserData,
				pResult->GetInteger(0));
	else
		pSelf->Score()->ShowTeamTop5(pResult, pResult->m_ClientID, pUserData);

#if defined(CONF_SQL)
	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
}

void CGameContext::ConTop5(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	//if (g_Config.m_SvInstagibMode)
	//{
	//	pSelf->Console()->Print(
	//		IConsole::OUTPUT_LEVEL_STANDARD,
	//		"rank",
	//		"Instagib ranks coming soon... check '/stats (name)' for now.");
	//}
	//else
	{

#if defined(CONF_SQL)
		if (pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
			if (pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery + pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
				return;
#endif

		if (g_Config.m_SvHideScore)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "top5",
				"Showing the top 5 is not allowed on this server.");
			return;
		}

		if (pResult->NumArguments() > 0 && pResult->GetInteger(0) >= 0)
			pSelf->Score()->ShowTop5(pResult, pResult->m_ClientID, pUserData,
				pResult->GetInteger(0));
		else
			pSelf->Score()->ShowTop5(pResult, pResult->m_ClientID, pUserData);

#if defined(CONF_SQL)
		if (pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
			pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
	}
}

#if defined(CONF_SQL)
void CGameContext::ConTimes(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckClientID(pResult->m_ClientID)) return;
	CGameContext *pSelf = (CGameContext *)pUserData;

#if defined(CONF_SQL)
	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		if(pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery + pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;
#endif

	if(g_Config.m_SvUseSQL)
	{
		CSqlScore *pScore = (CSqlScore *)pSelf->Score();
		CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
		if(!pPlayer)
			return;

		if(pResult->NumArguments() == 0)
		{
			pScore->ShowTimes(pPlayer->GetCID(),1);
			return;
		}

		else if(pResult->NumArguments() < 3)
		{
			if (pResult->NumArguments() == 1)
			{
				if(pResult->GetInteger(0) != 0)
					pScore->ShowTimes(pPlayer->GetCID(),pResult->GetInteger(0));
				else
					pScore->ShowTimes(pPlayer->GetCID(), (str_comp(pResult->GetString(0), "me") == 0) ? pSelf->Server()->ClientName(pResult->m_ClientID) : pResult->GetString(0),1);
				return;
			}
			else if (pResult->GetInteger(1) != 0)
			{
				pScore->ShowTimes(pPlayer->GetCID(), (str_comp(pResult->GetString(0), "me") == 0) ? pSelf->Server()->ClientName(pResult->m_ClientID) : pResult->GetString(0),pResult->GetInteger(1));
				return;
			}
		}

		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "times", "/times needs 0, 1 or 2 parameter. 1. = name, 2. = start number");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "times", "Example: /times, /times me, /times Hans, /times \"Papa Smurf\" 5");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "times", "Bad: /times Papa Smurf 5 # Good: /times \"Papa Smurf\" 5 ");

#if defined(CONF_SQL)
		if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
			pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
	}
}
#endif

void CGameContext::ConDND(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID)) return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if(!pPlayer)
		return;

	if(pPlayer->m_DND)
	{
		pPlayer->m_DND = false;
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "dnd", "You will receive global chat and server messages");
	}
	else
	{
		pPlayer->m_DND = true;
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "dnd", "You will not receive any further global chat and server messages");
	}
}

void CGameContext::ConMap(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	if (g_Config.m_SvMapVote == 0)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "map",
				"Admin has disabled /map");
		return;
	}

	if (pResult->NumArguments() <= 0)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "map", "Example: /map adr3 to call vote for Adrenaline 3. This means that the map name must start with 'a' and contain the characters 'd', 'r' and '3' in that order");
		return;
	}

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

#if defined(CONF_SQL)
	if(g_Config.m_SvUseSQL)
		if(pPlayer->m_LastSQLQuery + pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;
#endif

	pSelf->Score()->MapVote(pResult->m_ClientID, pResult->GetString(0));

#if defined(CONF_SQL)
	if(g_Config.m_SvUseSQL)
		pPlayer->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
}

void CGameContext::ConMapInfo(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

#if defined(CONF_SQL)
	if(g_Config.m_SvUseSQL)
		if(pPlayer->m_LastSQLQuery + pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;
#endif

	if (pResult->NumArguments() > 0)
		pSelf->Score()->MapInfo(pResult->m_ClientID, pResult->GetString(0));
	else
		pSelf->Score()->MapInfo(pResult->m_ClientID, g_Config.m_SvMap);

#if defined(CONF_SQL)
	if(g_Config.m_SvUseSQL)
		pPlayer->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
}

void CGameContext::ConTimeout(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	for(int i = 0; i < pSelf->Server()->MaxClients(); i++)
	{
		if (i == pResult->m_ClientID) continue;
		if (!pSelf->m_apPlayers[i]) continue;
		if (str_comp(pSelf->m_apPlayers[i]->m_TimeoutCode, pResult->GetString(0))) continue;
		if (((CServer *)pSelf->Server())->m_NetServer.SetTimedOut(i, pResult->m_ClientID))
		{
			((CServer *)pSelf->Server())->DelClientCallback(pResult->m_ClientID, "Timeout Protection used", ((CServer *)pSelf->Server()));
			((CServer *)pSelf->Server())->m_aClients[i].m_Authed = CServer::AUTHED_NO;
			if (pSelf->m_apPlayers[i]->GetCharacter())
				((CGameContext *)(((CServer *)pSelf->Server())->GameServer()))->SendTuningParams(i, pSelf->m_apPlayers[i]->GetCharacter()->m_TuneZone);
			return;
		}
	}

	((CServer *)pSelf->Server())->m_NetServer.SetTimeoutProtected(pResult->m_ClientID);
	str_copy(pPlayer->m_TimeoutCode, pResult->GetString(0), sizeof(pPlayer->m_TimeoutCode));
}

void CGameContext::ConSave(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

#if defined(CONF_SQL)
	if(!g_Config.m_SvSaveGames)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Save-function is disabled on this server");
		return;
	}

	if(g_Config.m_SvUseSQL)
		if(pPlayer->m_LastSQLQuery + pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;

	int Team = ((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.m_Core.Team(pResult->m_ClientID);

	const char* pCode = pResult->GetString(0);
	char aCountry[4];
	if(str_length(pCode) > 3 && pCode[0] >= 'A' && pCode[0] <= 'Z' && pCode[1] >= 'A'
		&& pCode[1] <= 'Z' && pCode[2] >= 'A' && pCode[2] <= 'Z' && pCode[3] == ' ')
	{
		str_copy(aCountry, pCode, 4);
		pCode = pCode + 4;
	}
	else
	{
		str_copy(aCountry, g_Config.m_SvSqlServerName, 4);
	}

	pSelf->Score()->SaveTeam(Team, pCode, pResult->m_ClientID, aCountry);

	if(g_Config.m_SvUseSQL)
		pPlayer->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
}

void CGameContext::ConLoad(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

#if defined(CONF_SQL)
	if(!g_Config.m_SvSaveGames)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Save-function is disabled on this server");
		return;
	}

	if(g_Config.m_SvUseSQL)
		if(pPlayer->m_LastSQLQuery + pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;
#endif

	if (pResult->NumArguments() > 0)
		pSelf->Score()->LoadTeam(pResult->GetString(0), pResult->m_ClientID);
	else
		return;

#if defined(CONF_SQL)
	if(g_Config.m_SvUseSQL)
		pPlayer->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
}

void CGameContext::ConTeamRank(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

#if defined(CONF_SQL)
	if(g_Config.m_SvUseSQL)
		if(pPlayer->m_LastSQLQuery + pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;
#endif

	if (pResult->NumArguments() > 0)
		if (!g_Config.m_SvHideScore)
			pSelf->Score()->ShowTeamRank(pResult->m_ClientID, pResult->GetString(0),
					true);
		else
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"teamrank",
					"Showing the team rank of other players is not allowed on this server.");
	else
		pSelf->Score()->ShowTeamRank(pResult->m_ClientID,
				pSelf->Server()->ClientName(pResult->m_ClientID));

#if defined(CONF_SQL)
	if(g_Config.m_SvUseSQL)
		pPlayer->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
}

void CGameContext::ConRank(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	//if (g_Config.m_SvInstagibMode)
	//{
	//	pSelf->Console()->Print(
	//		IConsole::OUTPUT_LEVEL_STANDARD,
	//		"rank",
	//		"Instagib ranks coming soon... check '/stats (name)' for now.");
	//}
	//else
	{

#if defined(CONF_SQL)
		if (g_Config.m_SvUseSQL)
			if (pPlayer->m_LastSQLQuery + pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
				return;
#endif

	if (pResult->NumArguments() > 0)
		if (!g_Config.m_SvHideScore)
			pSelf->Score()->ShowRank(pResult->m_ClientID, pResult->GetString(0),
				true);
		else
			pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"rank",
				"Showing the rank of other players is not allowed on this server.");
	else
		pSelf->Score()->ShowRank(pResult->m_ClientID,
			pSelf->Server()->ClientName(pResult->m_ClientID));

#if defined(CONF_SQL)
	if (g_Config.m_SvUseSQL)
		pPlayer->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
}
}



//moved to gamecontext.cpp cuz me knoop!
//void CGameContext::ConAddPolicehelper(IConsole::IResult *pResult, void *pUserData)
//{
//	CGameContext *pSelf = (CGameContext *)pUserData;
//	if (!CheckClientID(pResult->m_ClientID))
//		return;
//
//	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
//	if (!pPlayer)
//		return;
//
//
//
//	if (pResult->NumArguments() > 0)
//		if (!g_Config.m_SvHideScore)
//			pSelf->Score()->ShowRank(pResult->m_ClientID, pResult->GetString(0),
//				true);
//		else
//			pSelf->Console()->Print(
//				IConsole::OUTPUT_LEVEL_STANDARD,
//				"rank",
//				"Showing the rank of other players is not allowed on this server.");
//	else
//		pSelf->Console()->Print(
//			IConsole::OUTPUT_LEVEL_STANDARD,
//			"cmd-info",
//			"Error: Missing Arguments. Use following structure: /add_policehelper (player_name)");
//
//}


void CGameContext::ConLockTeam(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	int Team = ((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.m_Core.Team(pResult->m_ClientID);

	bool Lock = ((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.TeamLocked(Team);

	if (pResult->NumArguments() > 0)
		Lock = !pResult->GetInteger(0);

	if(Team > TEAM_FLOCK && Team < TEAM_SUPER)
	{
		char aBuf[512];
		if(Lock)
		{
			((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.SetTeamLock(Team, false);

			str_format(aBuf, sizeof(aBuf), "'%s' unlocked your team.", pSelf->Server()->ClientName(pResult->m_ClientID));

			for (int i = 0; i < MAX_CLIENTS; i++)
				if (((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.m_Core.Team(i) == Team)
					pSelf->SendChatTarget(i, aBuf);
		}
		else if(!g_Config.m_SvTeamLock)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"print",
					"Team locking is disabled on this server");
		}
		else
		{
			((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.SetTeamLock(Team, true);

			str_format(aBuf, sizeof(aBuf), "'%s' locked your team. After the race started killing will kill everyone in your team.", pSelf->Server()->ClientName(pResult->m_ClientID));

			for (int i = 0; i < MAX_CLIENTS; i++)
				if (((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.m_Core.Team(i) == Team)
					pSelf->SendChatTarget(i, aBuf);
		}
	}
	else
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"print",
				"This team can't be locked");
}

void CGameContext::ConJoinTeam(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (pSelf->m_VoteCloseTime && pSelf->m_VoteCreator == pResult->m_ClientID && (pSelf->m_VoteKick || pSelf->m_VoteSpec))
	{
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"join",
				"You are running a vote please try again after the vote is done!");
		return;
	}
	else if (g_Config.m_SvTeam == 0 || g_Config.m_SvTeam == 3)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
				"Admin has disabled teams");
		return;
	}
	else if (g_Config.m_SvTeam == 2 && pResult->GetInteger(0) == 0 && pPlayer->GetCharacter() && pPlayer->GetCharacter()->m_LastStartWarning < pSelf->Server()->Tick() - 3 * pSelf->Server()->TickSpeed())
	{
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"join",
				"You must join a team and play with somebody or else you can\'t play");
		pPlayer->GetCharacter()->m_LastStartWarning = pSelf->Server()->Tick();
	}

	if (pResult->NumArguments() > 0)
	{
		if (pPlayer->GetCharacter() == 0)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
					"You can't change teams while you are dead/a spectator.");
		}
		else
		{
			if (pPlayer->m_Last_Team
					+ pSelf->Server()->TickSpeed()
					* g_Config.m_SvTeamChangeDelay
					> pSelf->Server()->Tick())
			{
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
						"You can\'t change teams that fast!");
			}
			else if(pResult->GetInteger(0) > 0 && pResult->GetInteger(0) < MAX_CLIENTS && ((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.TeamLocked(pResult->GetInteger(0)))
			{
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
						"This team is locked using /lock. Only members of the team can unlock it using /lock.");
			}
			else if(pResult->GetInteger(0) > 0 && pResult->GetInteger(0) < MAX_CLIENTS && ((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.Count(pResult->GetInteger(0)) >= g_Config.m_SvTeamMaxSize)
			{
				char aBuf[512];
				str_format(aBuf, sizeof(aBuf), "This team already has the maximum allowed size of %d players", g_Config.m_SvTeamMaxSize);
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join", aBuf);
			}
			else if (((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.SetCharacterTeam(
					pPlayer->GetCID(), pResult->GetInteger(0)))
			{
				char aBuf[512];
				str_format(aBuf, sizeof(aBuf), "%s joined team %d",
						pSelf->Server()->ClientName(pPlayer->GetCID()),
						pResult->GetInteger(0));
				pSelf->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
				pPlayer->m_Last_Team = pSelf->Server()->Tick();
			}
			else
			{
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
						"You cannot join this team at this time");
			}
		}
	}
	else
	{
		char aBuf[512];
		if (!pPlayer->IsPlaying())
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"join",
					"You can't check your team while you are dead/a spectator.");
		}
		else
		{
			str_format(
					aBuf,
					sizeof(aBuf),
					"You are in team %d",
					((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.m_Core.Team(
							pResult->m_ClientID));
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
					aBuf);
		}
	}
}

void CGameContext::ConMe(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	char aBuf[256 + 24];

	str_format(aBuf, 256 + 24, "'%s' %s",
			pSelf->Server()->ClientName(pResult->m_ClientID),
			pResult->GetString(0));
	if (g_Config.m_SvSlashMe)
		pSelf->SendChat(-2, CGameContext::CHAT_ALL, aBuf, pResult->m_ClientID);
	else
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"me",
				"/me is disabled on this server, admin can enable it by using sv_slash_me");
}

void CGameContext::ConConverse(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	// This will never be called
}

void CGameContext::ConWhisper(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	// This will never be called
}

void CGameContext::ConSetEyeEmote(IConsole::IResult *pResult,
		void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	if(pResult->NumArguments() == 0) {
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"emote",
				(pPlayer->m_EyeEmote) ?
						"You can now use the preset eye emotes." :
						"You don't have any eye emotes, remember to bind some. (until you die)");
		return;
	}
	else if(str_comp_nocase(pResult->GetString(0), "on") == 0)
		pPlayer->m_EyeEmote = true;
	else if(str_comp_nocase(pResult->GetString(0), "off") == 0)
		pPlayer->m_EyeEmote = false;
	else if(str_comp_nocase(pResult->GetString(0), "toggle") == 0)
		pPlayer->m_EyeEmote = !pPlayer->m_EyeEmote;
	pSelf->Console()->Print(
			IConsole::OUTPUT_LEVEL_STANDARD,
			"emote",
			(pPlayer->m_EyeEmote) ?
					"You can now use the preset eye emotes." :
					"You don't have any eye emotes, remember to bind some. (until you die)");
}

void CGameContext::ConEyeEmote(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (g_Config.m_SvEmotionalTees == -1)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "emote",
				"Server admin disabled emotes.");
		return;
	}

	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (pResult->NumArguments() == 0)
	{
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"emote",
				"Emote commands are: /emote surprise /emote blink /emote close /emote angry /emote happy /emote pain");
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"emote",
				"Example: /emote surprise 10 for 10 seconds or /emote surprise (default 1 second)");
	}
	else
	{
			if(pPlayer->m_LastEyeEmote + g_Config.m_SvEyeEmoteChangeDelay * pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
				return;

			if (!str_comp(pResult->GetString(0), "angry"))
				pPlayer->m_DefEmote = EMOTE_ANGRY;
			else if (!str_comp(pResult->GetString(0), "blink"))
				pPlayer->m_DefEmote = EMOTE_BLINK;
			else if (!str_comp(pResult->GetString(0), "close"))
				pPlayer->m_DefEmote = EMOTE_BLINK;
			else if (!str_comp(pResult->GetString(0), "happy"))
				pPlayer->m_DefEmote = EMOTE_HAPPY;
			else if (!str_comp(pResult->GetString(0), "pain"))
				pPlayer->m_DefEmote = EMOTE_PAIN;
			else if (!str_comp(pResult->GetString(0), "surprise"))
				pPlayer->m_DefEmote = EMOTE_SURPRISE;
			else if (!str_comp(pResult->GetString(0), "normal"))
				pPlayer->m_DefEmote = EMOTE_NORMAL;
			else
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD,
						"emote", "Unknown emote... Say /emote");

			int Duration = 1;
			if (pResult->NumArguments() > 1)
				Duration = pResult->GetInteger(1);

			pPlayer->m_DefEmoteReset = pSelf->Server()->Tick()
							+ Duration * pSelf->Server()->TickSpeed();
			pPlayer->m_LastEyeEmote = pSelf->Server()->Tick();
	}
}

void CGameContext::ConNinjaJetpack(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	if (pResult->NumArguments())
		pPlayer->m_NinjaJetpack = pResult->GetInteger(0);
	else
		pPlayer->m_NinjaJetpack = !pPlayer->m_NinjaJetpack;
}

void CGameContext::ConShowOthers(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	if (g_Config.m_SvShowOthers)
	{
		if (pResult->NumArguments())
			pPlayer->m_ShowOthers = pResult->GetInteger(0);
		else
			pPlayer->m_ShowOthers = !pPlayer->m_ShowOthers;
	}
	else
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"showotherschat",
				"Showing players from other teams is disabled by the server admin");
}

void CGameContext::ConShowAll(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (pResult->NumArguments())
		pPlayer->m_ShowAll = pResult->GetInteger(0);
	else
		pPlayer->m_ShowAll = !pPlayer->m_ShowAll;
}

void CGameContext::ConSpecTeam(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (pResult->NumArguments())
		pPlayer->m_SpecTeam = pResult->GetInteger(0);
	else
		pPlayer->m_SpecTeam = !pPlayer->m_SpecTeam;
}

bool CheckClientID(int ClientID)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	dbg_assert(ClientID >= 0 || ClientID < MAX_CLIENTS,
			"The Client ID is wrong");
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;
	return true;
}

void CGameContext::ConSayTime(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	int ClientID;
	char aBufname[MAX_NAME_LENGTH];

	if (pResult->NumArguments() > 0)
	{
		for(ClientID = 0; ClientID < MAX_CLIENTS; ClientID++)
			if (str_comp(pResult->GetString(0), pSelf->Server()->ClientName(ClientID)) == 0)
				break;

		if(ClientID == MAX_CLIENTS)
			return;

		str_format(aBufname, sizeof(aBufname), "%s's", pSelf->Server()->ClientName(ClientID));
	}
	else
	{
		str_copy(aBufname, "Your", sizeof(aBufname));
		ClientID = pResult->m_ClientID;
	}

	CPlayer *pPlayer = pSelf->m_apPlayers[ClientID];
	if (!pPlayer)
		return;
	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;
	if(pChr->m_DDRaceState != DDRACE_STARTED)
		return;

	char aBuftime[64];
	int IntTime = (int) ((float) (pSelf->Server()->Tick() - pChr->m_StartTime)
			/ ((float) pSelf->Server()->TickSpeed()));
	str_format(aBuftime, sizeof(aBuftime), "%s time is %s%d:%s%d",
			aBufname,
			((IntTime / 60) > 9) ? "" : "0", IntTime / 60,
			((IntTime % 60) > 9) ? "" : "0", IntTime % 60);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "time", aBuftime);
}

void CGameContext::ConSayTimeAll(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;
	if(pChr->m_DDRaceState != DDRACE_STARTED)
		return;

	char aBuftime[64];
	int IntTime = (int) ((float) (pSelf->Server()->Tick() - pChr->m_StartTime)
			/ ((float) pSelf->Server()->TickSpeed()));
	str_format(aBuftime, sizeof(aBuftime),
			"%s\'s current race time is %s%d:%s%d",
			pSelf->Server()->ClientName(pResult->m_ClientID),
			((IntTime / 60) > 9) ? "" : "0", IntTime / 60,
			((IntTime % 60) > 9) ? "" : "0", IntTime % 60);
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, aBuftime, pResult->m_ClientID);
}

void CGameContext::ConTime(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	char aBuftime[64];
	int IntTime = (int) ((float) (pSelf->Server()->Tick() - pChr->m_StartTime)
			/ ((float) pSelf->Server()->TickSpeed()));
	str_format(aBuftime, sizeof(aBuftime), "Your time is %s%d:%s%d",
				((IntTime / 60) > 9) ? "" : "0", IntTime / 60,
				((IntTime % 60) > 9) ? "" : "0", IntTime % 60);
	pSelf->SendBroadcast(aBuftime, pResult->m_ClientID);
}

void CGameContext::ConSetTimerType(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;

	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	const char msg[3][128] = {"game/round timer.", "broadcast.", "both game/round timer and broadcast."};
	char aBuf[128];
	if(pPlayer->m_TimerType <= 2 && pPlayer->m_TimerType >= 0)
		str_format(aBuf, sizeof(aBuf), "Timer is displayed in", msg[pPlayer->m_TimerType]);
	else if(pPlayer->m_TimerType == 3)
		str_format(aBuf, sizeof(aBuf), "Timer isn't displayed.");

	if(pResult->NumArguments() == 0) {
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD,"timer",aBuf);
		return;
	}
	else if(str_comp_nocase(pResult->GetString(0), "gametimer") == 0) {
		pSelf->SendBroadcast("", pResult->m_ClientID);
		pPlayer->m_TimerType = 0;
	}
	else if(str_comp_nocase(pResult->GetString(0), "broadcast") == 0)
			pPlayer->m_TimerType = 1;
	else if(str_comp_nocase(pResult->GetString(0), "both") == 0)
			pPlayer->m_TimerType = 2;
	else if(str_comp_nocase(pResult->GetString(0), "none") == 0)
			pPlayer->m_TimerType = 3;
	else if(str_comp_nocase(pResult->GetString(0), "cycle") == 0) {
		if(pPlayer->m_TimerType < 3)
			pPlayer->m_TimerType++;
		else if(pPlayer->m_TimerType == 3)
			pPlayer->m_TimerType = 0;
	}
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD,"timer",aBuf);
}

void CGameContext::ConRescue(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	if (!g_Config.m_SvRescue) {
		pSelf->SendChatTarget(pPlayer->GetCID(), "Rescue is not enabled on this server");
		return;
	}

	pChr->Rescue();
}

void CGameContext::ConProtectedKill(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	int CurrTime = (pSelf->Server()->Tick() - pChr->m_StartTime) / pSelf->Server()->TickSpeed();
	if(g_Config.m_SvKillProtection != 0 && CurrTime >= (60 * g_Config.m_SvKillProtection) && pChr->m_DDRaceState == DDRACE_STARTED)
	{
			pPlayer->KillCharacter(WEAPON_SELF);

			//char aBuf[64];
			//str_format(aBuf, sizeof(aBuf), "You killed yourself in: %s%d:%s%d",
			//		((CurrTime / 60) > 9) ? "" : "0", CurrTime / 60,
			//		((CurrTime % 60) > 9) ? "" : "0", CurrTime % 60);
			//pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
}
//#if defined(CONF_SQL)
void CGameContext::ConPoints(IConsole::IResult *pResult, void *pUserData)
{
	if (g_Config.m_SvPointsMode == 1) //ddnet
	{
#if defined(CONF_SQL)
		CGameContext *pSelf = (CGameContext *)pUserData;
		if (!CheckClientID(pResult->m_ClientID))
			return;

		if (pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
			if (pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery + pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
				return;

		CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
		if (!pPlayer)
			return;

		if (pResult->NumArguments() > 0)
			if (!g_Config.m_SvHideScore)
				pSelf->Score()->ShowPoints(pResult->m_ClientID, pResult->GetString(0),
					true);
			else
				pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"points",
					"Showing the global points of other players is not allowed on this server.");
		else
			pSelf->Score()->ShowPoints(pResult->m_ClientID,
				pSelf->Server()->ClientName(pResult->m_ClientID));

		if (pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
			pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery = pSelf->Server()->Tick();
#else
		CGameContext *pSelf = (CGameContext *)pUserData;
		if (!CheckClientID(pResult->m_ClientID))
			return;
		CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
		if (!pPlayer)
			return;

		pSelf->Console()->Print(
			IConsole::OUTPUT_LEVEL_STANDARD,
			"points",
			"This is not an SQL server.");
#endif
	}
	else if (g_Config.m_SvPointsMode == 2) //ddpp (blockpoints)
	{
		CGameContext *pSelf = (CGameContext *)pUserData;
		if (!CheckClientID(pResult->m_ClientID))
			return;
		CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
		if (!pPlayer)
			return;

		char aBuf[256];

		if (pResult->NumArguments() > 0) //show others
		{
			int pointsID = pSelf->GetCIDByName(pResult->GetString(0));
			if (pointsID == -1)
			{
				str_format(aBuf, sizeof(aBuf), "'%s' is not online", pResult->GetString(0));
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
				return;
			}


			pSelf->SendChatTarget(pResult->m_ClientID, "---- BLOCK STATS ----");
			str_format(aBuf, sizeof(aBuf), "Points: %d", pSelf->m_apPlayers[pointsID]->m_BlockPoints);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Kills: %d", pSelf->m_apPlayers[pointsID]->m_BlockPoints_Kills);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Deaths: %d", pSelf->m_apPlayers[pointsID]->m_BlockPoints_Deaths);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		}
		else //show own
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "---- BLOCK STATS ----");
			str_format(aBuf, sizeof(aBuf), "Points: %d", pPlayer->m_BlockPoints);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Kills: %d", pPlayer->m_BlockPoints_Kills);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Deaths: %d", pPlayer->m_BlockPoints_Deaths);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		}
	}
	else //points deactivated
	{
		CGameContext *pSelf = (CGameContext *)pUserData;
		if (!CheckClientID(pResult->m_ClientID))
			return;
		CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
		if (!pPlayer)
			return;

		pSelf->Console()->Print(
			IConsole::OUTPUT_LEVEL_STANDARD,
			"points",
			"showing points is deactivated on this DDnet++ server.");
	}
}
//#endif

#if defined(CONF_SQL)
void CGameContext::ConTopPoints(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		if(pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery + pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;

	if (g_Config.m_SvHideScore)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "toppoints",
				"Showing the global top points is not allowed on this server.");
		return;
	}

	if (pResult->NumArguments() > 0 && pResult->GetInteger(0) >= 0)
		pSelf->Score()->ShowTopPoints(pResult, pResult->m_ClientID, pUserData,
				pResult->GetInteger(0));
	else
		pSelf->Score()->ShowTopPoints(pResult, pResult->m_ClientID, pUserData);

	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery = pSelf->Server()->Tick();
}
#endif


void CGameContext::ConPolicetaser(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	if (pPlayer->m_TaserLevel < 1)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "You don't own a taser.");
		return;
	}

	if (pResult->NumArguments() != 1)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Invalid. Type '/policetaser on' or '/policetaser off'");
		return;
	}

	char aInput[32];
	str_copy(aInput, pResult->GetString(0), 32);

	if (!str_comp_nocase(aInput, "on"))
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Taser activated. (Your rifle is now a taser)");
		pPlayer->m_TaserOn = true;
		return;
	}
	else if (!str_comp_nocase(aInput, "off"))
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Taser deactivated. (Your rifle unfreezes people)");
		pPlayer->m_TaserOn = false;
		return;
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Invalid. Type '/policetaser on' or '/policetaser off'");
		return;
	}
}

void CGameContext::ConBuy(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;


	if (pResult->NumArguments() != 1)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Unknown item. Type '/buy <itemname>' use '/shop' to see the full itemlist.");
		return;
	}


	char aBuf[256];
	char aItem[32];
	str_copy(aItem, pResult->GetString(0), 32);

	if (!str_comp_nocase(aItem, "police"))
	{
		//pSelf->SendChatTarget(pResult->m_ClientID, "PoliceRank isnt available yet.");
		//return;

		if (pPlayer->m_PoliceRank == 0)
		{
			if (pPlayer->m_level < 18)
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Your level is too low! You need level 18 to buy police.");
				return;
			}
		}
		else if (pPlayer->m_PoliceRank == 1)
		{
			if (pPlayer->m_level < 25)
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Your level is too low! You need level 25 to upgrade police to level 2.");
				return;
			}
		}
		else if (pPlayer->m_PoliceRank == 2)
		{
			if (pPlayer->m_level < 30)
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Your level is too low! You need level 30 to upgrade police to level 3.");
				return;
			}
		}


		if (pPlayer->m_PoliceRank > 2)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You already bought maximum police lvl.");
			return;
		}

		if (pPlayer->m_money >= 100000)
		{
			pPlayer->MoneyTransaction(-100000, "-100000 you bought shit");
			pPlayer->m_PoliceRank++;
			str_format(aBuf, sizeof(aBuf), "You bought PoliceRank[%d]!", pPlayer->m_PoliceRank);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You don't have enough money! You need 100 000.");
		}
	}
	else if (!str_comp_nocase(aItem, "taser"))
	{
		//TODO / coming soon...:
		//add taser levels...
		//make taster shitty and sometimes dont work and higher levels lower the shitty risk

		//pSelf->SendChatTarget(pResult->m_ClientID, "coming soon...");
		//return;


		if (pPlayer->m_PoliceRank < 3)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You don't own a weapon license.");
			return;
		}

		if (pPlayer->m_TaserLevel == 0)
		{
			pPlayer->m_TaserPrice = 50000;
		}
		else if (pPlayer->m_TaserLevel == 1)
		{
			pPlayer->m_TaserPrice = 75000;
		}
		else if (pPlayer->m_TaserLevel == 2)
		{
			pPlayer->m_TaserPrice = 100000;
		}
		else if (pPlayer->m_TaserLevel == 3)
		{
			pPlayer->m_TaserPrice = 150000;
		}
		else if (pPlayer->m_TaserLevel == 4)
		{
			pPlayer->m_TaserPrice = 200000;
		}
		else if (pPlayer->m_TaserLevel == 5)
		{
			pPlayer->m_TaserPrice = 200000;
		}
		else if (pPlayer->m_TaserLevel == 6)
		{
			pPlayer->m_TaserPrice = 200000;
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You already bought maximum taser level.");
			return;
		}

		if (pPlayer->m_money < pPlayer->m_TaserPrice)
		{
			str_format(aBuf, sizeof(aBuf), "You don't have enough money to upgrade your taser. You need %d money", pPlayer->m_TaserPrice);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			return;
		}

		str_format(aBuf, sizeof(aBuf), "-%d bought taser", pPlayer->m_TaserPrice);
		pPlayer->MoneyTransaction(-pPlayer->m_TaserPrice, aBuf);


		pPlayer->m_TaserLevel++;
		if (pPlayer->m_TaserLevel == 1)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You bought a taser. (use '/policetaser on' to activate it)");
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You upgraded your taser.");
		}
	}
	else if (!str_comp_nocase(aItem, "shit"))
	{
		if (pPlayer->m_money >= 5)
		{
			pPlayer->MoneyTransaction(-5, "-5 you bought shit");

			pPlayer->m_shit++;
			pSelf->SendChatTarget(pResult->m_ClientID, "You bought shit.");
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You don't have enough money!");
		}
	}
	else if (!str_comp_nocase(aItem, "room_key"))
	{
		if (pPlayer->m_level < 16)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You need level 16 or more to buy a key.");
			return;
		}
		if (pPlayer->m_BoughtRoom)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You already own this item.");
			return;
		}
		if (g_Config.m_SvRoomState == 0)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "An admin has turned off the room.");
			return;
		}
		if (pPlayer->m_money >= g_Config.m_SvRoomPrice)
		{
			str_format(aBuf, sizeof(aBuf), "-%d bought room_key", g_Config.m_SvRoomPrice);
			pPlayer->MoneyTransaction(-g_Config.m_SvRoomPrice, aBuf);
			pPlayer->m_BoughtRoom = true;
			pSelf->SendChatTarget(pResult->m_ClientID, "You bought a key. You can now enter the bankroom until disconnect.");
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You don't have enough money! You need 5 000.");
		}
	}
	else if (!str_comp_nocase(aItem, "chidraqul"))
	{
		if (pPlayer->m_level < 2)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You need level 2 or more to buy the minigame chidraqul.");
			return;
		}


		if (pPlayer->m_BoughtGame)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You already own this game.");
			return;
		}


		if (pPlayer->m_money >= 250)
		{
			pPlayer->MoneyTransaction(-250, "-250 bought chidraqul.");
			pPlayer->m_BoughtGame = true;
			pSelf->SendChatTarget(pResult->m_ClientID, "You bought the minigame chidraqul until disconnect. Check '/minigameinfo' for more information.");
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You don't have enough money! You need 250.");
		}
	}
	// buy cosmetic feature
	else if (!str_comp_nocase(aItem, "rainbow"))
	{
		if (pPlayer->GetCharacter()->m_Rainbow)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You already own rainbow");
			return;
		}

		if (pPlayer->m_level < 5)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Your level is too low! You need level 5 to buy rainbow.");
		}
		else
		{
			if (pPlayer->m_money >= 1500)
			{
				pPlayer->MoneyTransaction(-1500, "-1500 bought rainbow");
				pPlayer->GetCharacter()->m_Rainbow = true;
				pSelf->SendChatTarget(pResult->m_ClientID, "You bought rainbow until death.");
			}
			else
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "You don't have enough money! You need 1 500.");
			}
		}
	}
	else if (!str_comp_nocase(aItem, "bloody"))
	{
		if (pPlayer->GetCharacter()->m_Bloody)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You already own bloody");
			return;
		}

		if (pPlayer->m_level < 15)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Your level is too low! You need level 15 to buy bloody.");
		}
		else
		{
			if (pPlayer->m_money >= 3500)
			{
				pPlayer->MoneyTransaction(-3500, "-3500 bought bloody");
				pPlayer->GetCharacter()->m_Bloody = true;
				pSelf->SendChatTarget(pResult->m_ClientID, "You bought bloody until death.");
			}
			else
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "You don't have enough money! You need 3 500.");
			}
		}
	}
	/*else if (!str_comp_nocase(aItem, "atom"))
	{
		if (pPlayer->GetCharacter()->m_Atom)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "you already own atom");
			return;
		}

		if (pPlayer->m_level < 15)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "your level is too low! you need level 15 to buy atom.");
		}
		else
		{
			if (pPlayer->m_money >= 3500)
			{
				pPlayer->MoneyTransaction(-3500, "-3500 bought pvp_arena_ticket");
				pPlayer->GetCharacter()->m_Atom = true;
				pSelf->SendChatTarget(pResult->m_ClientID, "you bought atom until death.");
			}
			else
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "you don't have enough money! You need 3 500.");
			}
		}
	}
	else if (!str_comp_nocase(aItem, "trail"))
	{
		if (pPlayer->GetCharacter()->m_Trail)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "you already own trail");
			return;
		}

		if (pPlayer->m_level < 15)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "your level is too low! you need level 15 to buy trail.");
		}
		else
		{
			if (pPlayer->m_money >= 3500)
			{
				pPlayer->MoneyTransaction(-3500, "-3500 bought pvp_arena_ticket");
				pPlayer->GetCharacter()->m_Trail = true;
				pSelf->SendChatTarget(pResult->m_ClientID, "you bought trail until death.");
			}
			else
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "you don't have enough money! You need 3 500.");
			}
		}
	}*/
	else if (!str_comp_nocase(aItem, "pvp_arena_ticket"))
	{
		if (pPlayer->m_money >= 150)
		{
			pPlayer->MoneyTransaction(-150, "-150 bought pvp_arena_ticket");
			pPlayer->m_pvp_arena_tickets++;

			str_format(aBuf, sizeof(aBuf), "You bought a pvp_arena_ticket. You have %d tickets.", pPlayer->m_pvp_arena_tickets);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You don't have enough money! You need 150.");
		}
	}
	//else if (!str_comp_nocase(aItem, "bloody"))
	//{
	//	if (pChr->m_BoughtBloody)
	//		pSelf->SendChatTarget(pResult->m_ClientID, "You already bought bloody");
	//	else if (pPlayer->m_money >= 100)
	//	{
	//		pSelf->SendChatTarget(pResult->m_ClientID, "you bought bloody until death.");
	//		pPlayer->m_money -= 100;
	//		pChr->m_BoughtBloody = true;
	//	}
	//	else
	//	{
	//		pSelf->SendChatTarget(pResult->m_ClientID, "you don't have enough money!");
	//	}
	//}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Invalid shop item. Choose another one.");
	}

}

void CGameContext::ConRegister(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	int ClientID = pResult->m_ClientID;
	CPlayer *pPlayer = pSelf->m_apPlayers[ClientID];
	if (!pPlayer)
		return;

	if (g_Config.m_SvAccountStuff == 0)
	{
		pSelf->SendChatTarget(ClientID, "Accounts are turned off on this server.");
		return;
	}


	if (pResult->NumArguments() != 3)
	{
		pSelf->SendChatTarget(ClientID, "Please use '/register <name> <password> <password>'.");
		return;
	}

	if (pPlayer->m_AccountID > 0)
	{
		pSelf->SendChatTarget(ClientID, "You are already logged in.");
		return;
	}

	char aUsername[32];
	char aPassword[128];
	char aPassword2[128];
	str_copy(aUsername, pResult->GetString(0), sizeof(aUsername));
	str_copy(aPassword, pResult->GetString(1), sizeof(aPassword));
	str_copy(aPassword2, pResult->GetString(2), sizeof(aPassword2));

	if (str_length(aUsername) > 20 || str_length(aUsername) < 3)
	{
		pSelf->SendChatTarget(ClientID, "Your Username is too long or too short. Max length 20 Min length 3");
		return;
	}

	if ((str_length(aPassword) > 20 || str_length(aPassword) < 3) || (str_length(aPassword2) > 20 || str_length(aPassword2) < 3))
	{
		pSelf->SendChatTarget(ClientID, "Your Password is too long or too short. Max length 20 Min length 3");
		return;
	}

	if (str_comp_nocase(aPassword, aPassword2) != 0)
	{
		pSelf->SendChatTarget(ClientID, "Passwords needs to be the same.");
		return;
	}

	char *pQueryBuf = sqlite3_mprintf("SELECT * FROM Accounts WHERE Username='%q'", aUsername);
	CQueryRegister *pQuery = new CQueryRegister();
	pQuery->m_ClientID = ClientID;
	pQuery->m_pGameServer = pSelf;
	pQuery->m_Name = aUsername;
	pQuery->m_Password = aPassword;
	pQuery->Query(pSelf->m_Database, pQueryBuf);
	sqlite3_free(pQueryBuf);
}

void CGameContext::ConSQL(IConsole::IResult * pResult, void * pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	int ClientID = pResult->m_ClientID;
	CPlayer *pPlayer = pSelf->m_apPlayers[ClientID];
	if (!pPlayer)
		return;

	if (g_Config.m_SvAccountStuff == 0)
	{
		pSelf->SendChatTarget(ClientID, "account stuff is turned off on this server.");
		return;
	}

	//if (pResult->NumArguments() < 2)
	//{
	//	pSelf->SendChatTarget(ClientID, "Error: si?i");
	//	return;
	//}

	if (pPlayer->m_Authed != CServer::AUTHED_ADMIN) //after Arguments check to troll curious users
	{
		pSelf->SendChatTarget(ClientID, "missing permission to use this command.");
		return;
	}

	if (pResult->NumArguments() == 0)
	{
		pSelf->SendChatTarget(ClientID, "---- commands -----");
		pSelf->SendChatTarget(ClientID, "'/sql getid <clientid>' to get sql id");
		pSelf->SendChatTarget(ClientID, "'/sql super_mod <sqlid> <val>'");
		pSelf->SendChatTarget(ClientID, "'/sql mod <sqlid> <val>'");
		pSelf->SendChatTarget(ClientID, "'/sql freeze_acc <sqlid> <val>'");
		pSelf->SendChatTarget(ClientID, "----------------------");
		pSelf->SendChatTarget(ClientID, "'/acc_info <clientID>' additional info");
		return;
	}

	char aBuf[128];
	char aCommand[32];
	int SQL_ID;
	str_copy(aCommand, pResult->GetString(0), sizeof(aCommand));
	SQL_ID = pResult->GetInteger(1);


	if (!str_comp_nocase(aCommand, "getid")) //2 argument commands
	{
		if (!pSelf->m_apPlayers[SQL_ID])
		{
			str_format(aBuf, sizeof(aBuf), "No player with the id '%d' online.", SQL_ID);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			return;
		}

		if (pSelf->m_apPlayers[SQL_ID]->m_AccountID <= 0)
		{
			str_format(aBuf, sizeof(aBuf), "Player '%s' is not logged in.", pSelf->Server()->ClientName(SQL_ID));
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			return;
		}

		str_format(aBuf, sizeof(aBuf), "'%s' SQL-ID: %d", pSelf->Server()->ClientName(SQL_ID), pSelf->m_apPlayers[SQL_ID]->m_AccountID);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
	else if (!str_comp_nocase(aCommand, "help"))
	{
		pSelf->SendChatTarget(ClientID, "---- commands -----");
		pSelf->SendChatTarget(ClientID, "'/sql getid <clientid>' to get sql id");
		pSelf->SendChatTarget(ClientID, "'/sql super_mod <sqlid> <val>'");
		pSelf->SendChatTarget(ClientID, "'/sql mod <sqlid> <val>'");
		pSelf->SendChatTarget(ClientID, "'/sql freeze_acc <sqlid> <val>'");
		pSelf->SendChatTarget(ClientID, "----------------------");
		pSelf->SendChatTarget(ClientID, "'/acc_info <clientID>' additional info");
	}
	else if (!str_comp_nocase(aCommand, "super_mod"))
	{
		if (pResult->NumArguments() < 3)
		{
			pSelf->SendChatTarget(ClientID, "Error: sql <command> <id> <value>");
			return;
		}
		int value;
		value = pResult->GetInteger(2);

		char *pQueryBuf = sqlite3_mprintf("UPDATE Accounts SET IsSuperModerator='%d' WHERE ID='%d'", value, SQL_ID);

		CQuery *pQuery = new CQuery();
		pQuery->Query(pSelf->m_Database, pQueryBuf);
		sqlite3_free(pQueryBuf);


		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (pSelf->m_apPlayers[i])
			{
				if (pSelf->m_apPlayers[i]->m_AccountID == SQL_ID)
				{
					pSelf->m_apPlayers[i]->m_IsSuperModerator = value;
					if (value == 1)
					{
						pSelf->SendChatTarget(i, "You are now SuperModerator.");
					}
					else
					{
						pSelf->SendChatTarget(i, "You are no longer SuperModerator.");
					}
					str_format(aBuf, sizeof(aBuf), "UPDATED IsSuperModerator = %d (account is logged in)", value);
					pSelf->SendChatTarget(ClientID, aBuf);
					return;
				}
			}
		}
		str_format(aBuf, sizeof(aBuf), "UPDATED IsSuperModerator = %d (account is not logged in)", value);
		pSelf->SendChatTarget(ClientID, aBuf);
	}
	else if (!str_comp_nocase(aCommand, "mod"))
	{
		if (pResult->NumArguments() < 3)
		{
			pSelf->SendChatTarget(ClientID, "Error: sql <command> <id> <value>");
			return;
		}
		int value;
		value = pResult->GetInteger(2);

		char *pQueryBuf = sqlite3_mprintf("UPDATE Accounts SET IsModerator='%d' WHERE ID='%d'", value, SQL_ID);

		CQuery *pQuery = new CQuery();
		pQuery->Query(pSelf->m_Database, pQueryBuf);
		sqlite3_free(pQueryBuf);

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (pSelf->m_apPlayers[i])
			{
				if (pSelf->m_apPlayers[i]->m_AccountID == SQL_ID)
				{
					pSelf->m_apPlayers[i]->m_IsModerator = value;
					if (value == 1)
					{
						pSelf->SendChatTarget(i, "You are now Moderator.");
					}
					else
					{
						pSelf->SendChatTarget(i, "You are no longer Moderator.");
					}
					str_format(aBuf, sizeof(aBuf), "UPDATED IsModerator = %d (account is logged in)", value);
					pSelf->SendChatTarget(ClientID, aBuf);
					return;
				}
			}
		}
		str_format(aBuf, sizeof(aBuf), "UPDATED IsModerator = %d (account is not logged in)", value);
		pSelf->SendChatTarget(ClientID, aBuf);
	}
	else if (!str_comp_nocase(aCommand, "freeze_acc"))
	{
		if (pResult->NumArguments() < 3)
		{
			pSelf->SendChatTarget(ClientID, "Error: sql <command> <id> <value>");
			return;
		}
		int value;
		value = pResult->GetInteger(2);

		char *pQueryBuf = sqlite3_mprintf("UPDATE Accounts SET IsAccFrozen='%d' WHERE ID='%d'", value, SQL_ID);

		CQuery *pQuery = new CQuery();
		pQuery->Query(pSelf->m_Database, pQueryBuf);
		sqlite3_free(pQueryBuf);

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (pSelf->m_apPlayers[i])
			{
				if (pSelf->m_apPlayers[i]->m_AccountID == SQL_ID)
				{
					pSelf->m_apPlayers[i]->m_IsAccFrozen = value;
					pSelf->m_apPlayers[i]->Logout(); //always logout and send you got frozen also if he gets unfreezed because if some1 gets unfreezed he is not logged in xd
					pSelf->SendChatTarget(i, "Logged out. (Reason: Account frozen)");
					str_format(aBuf, sizeof(aBuf), "UPDATED IsAccFrozen = %d (account is logged in)", value);
					pSelf->SendChatTarget(ClientID, aBuf);
					return;
				}
			}
		}
		str_format(aBuf, sizeof(aBuf), "UPDATED IsAccFrozen = %d (account is not logged in)", value);
		pSelf->SendChatTarget(ClientID, aBuf);
	}
	else 
	{
		pSelf->SendChatTarget(ClientID, "Unknown SQL command. Try '/SQL help' for more help.");
	}

}

void CGameContext::ConAcc_Info(IConsole::IResult * pResult, void * pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	int ClientID = pResult->m_ClientID;
	CPlayer *pPlayer = pSelf->m_apPlayers[ClientID];
	if (!pPlayer)
		return;

	if (g_Config.m_SvAccountStuff == 0)
	{
		pSelf->SendChatTarget(ClientID, "account stuff is turned off on this server.");
		return;
	}


	if (pPlayer->m_Authed != CServer::AUTHED_ADMIN)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission.");
		return;
	}

	if (pResult->NumArguments() != 1)
	{
		pSelf->SendChatTarget(ClientID, "Please use '/acc_info <name>'.");
		return;
	}

	char aUsername[32];
	int InfoID = -1;
	str_copy(aUsername, pResult->GetString(0), sizeof(aUsername));

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (!str_comp_nocase(aUsername, pSelf->Server()->ClientName(i)))
		{
			InfoID = i;
		}
	}

	if (InfoID > -1)
	{
		if (pSelf->m_apPlayers[InfoID]->m_AccountID <= 0)
		{
			pSelf->SendChatTarget(ClientID, "This player is not logged in.");
			return;
		}

		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "==== Name: %s SQL: %d ====", pSelf->Server()->ClientName(pSelf->m_apPlayers[InfoID]->GetCID()), pSelf->m_apPlayers[InfoID]->m_AccountID);
		pSelf->SendChatTarget(ClientID, aBuf);
		pSelf->SendChatTarget(ClientID, pSelf->m_apPlayers[InfoID]->m_LastLogoutIGN1);
		pSelf->SendChatTarget(ClientID, pSelf->m_apPlayers[InfoID]->m_LastLogoutIGN2);
		pSelf->SendChatTarget(ClientID, pSelf->m_apPlayers[InfoID]->m_LastLogoutIGN3);
		pSelf->SendChatTarget(ClientID, pSelf->m_apPlayers[InfoID]->m_LastLogoutIGN4);
		pSelf->SendChatTarget(ClientID, pSelf->m_apPlayers[InfoID]->m_LastLogoutIGN5);
	}
	else
	{
		pSelf->SendChatTarget(ClientID, "Unkown player name.");
	}
}

void CGameContext::ConStats(IConsole::IResult * pResult, void * pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	//CCharacter* pChr = pPlayer->GetCharacter();
	//if (!pChr)
	//	return;

	char aBuf[512];

	if (g_Config.m_SvInstagibMode) //pvp stats
	{
		if (pResult->NumArguments() > 0) //other players stats
		{
			char aStatsName[32];
			str_copy(aStatsName, pResult->GetString(0), sizeof(aStatsName));
			int StatsID = pSelf->GetCIDByName(aStatsName);
			if (StatsID == -1)
			{
				str_format(aBuf, sizeof(aBuf), "Can't find user '%s'", aStatsName);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
				return;
			}
			//if (pSelf->m_apPlayers[StatsID]->m_AccountID < 1)
			//{
			//	str_format(aBuf, sizeof(aBuf), "'%s' is not logged in.", aStatsName);
			//	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			//	return;
			//}

			str_format(aBuf, sizeof(aBuf), "====== %s's Stats ======", aStatsName);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendChatTarget(pResult->m_ClientID, "~~~ Grenade instagib ~~~");
			str_format(aBuf, sizeof(aBuf), "Kills: %d", pSelf->m_apPlayers[StatsID]->m_GrenadeKills);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Deaths: %d", pSelf->m_apPlayers[StatsID]->m_GrenadeDeaths);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Highest spree: %d", pSelf->m_apPlayers[StatsID]->m_GrenadeSpree);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Total shots: %d", pSelf->m_apPlayers[StatsID]->m_GrenadeShots);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			//str_format(aBuf, sizeof(aBuf), "Shots without RJ: %d", pSelf->m_apPlayers[StatsID]->m_GrenadeShotsNoRJ);
			//pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendChatTarget(pResult->m_ClientID, "~~~ Rifle instagib ~~~");
			str_format(aBuf, sizeof(aBuf), "Kills: %d", pSelf->m_apPlayers[StatsID]->m_RifleKills);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Deaths: %d", pSelf->m_apPlayers[StatsID]->m_RifleDeaths);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Highest spree: %d", pSelf->m_apPlayers[StatsID]->m_RifleSpree);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Total shots: %d", pSelf->m_apPlayers[StatsID]->m_RifleShots);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);

		}
		else //own stats
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "====== Your Stats ======");
			pSelf->SendChatTarget(pResult->m_ClientID, "~~~ Grenade instagib ~~~");
			str_format(aBuf, sizeof(aBuf), "Kills: %d", pPlayer->m_GrenadeKills);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Deaths: %d", pPlayer->m_GrenadeDeaths);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Highest spree: %d", pPlayer->m_GrenadeSpree);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Total shots: %d", pPlayer->m_GrenadeShots);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Shots without RJ: %d", pPlayer->m_GrenadeShotsNoRJ);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Rocketjumps: %d", pPlayer->m_GrenadeShots - pPlayer->m_GrenadeShotsNoRJ);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			//str_format(aBuf, sizeof(aBuf), "Failed shots (no kill, no rj): %d", pPlayer->m_GrenadeShots - (pPlayer->m_GrenadeShots - pPlayer->m_GrenadeShotsNoRJ) - pPlayer->m_GrenadeKills); //can be negative with double and tripple kills but this isnt a bug its a feature xd
			//pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendChatTarget(pResult->m_ClientID, "~~~ Rifle instagib ~~~");
			str_format(aBuf, sizeof(aBuf), "Kills: %d", pPlayer->m_RifleKills);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Deaths: %d", pPlayer->m_RifleDeaths);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Highest spree: %d", pPlayer->m_RifleSpree);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Total shots: %d", pPlayer->m_RifleShots);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		}
	}
	else //blockcity stats
	{
		if (pResult->NumArguments() > 0) //other players stats
		{
			char aStatsName[32];
			str_copy(aStatsName, pResult->GetString(0), sizeof(aStatsName));
			int StatsID = pSelf->GetCIDByName(aStatsName);
			if (StatsID == -1)
			{
				str_format(aBuf, sizeof(aBuf), "Can't find user '%s'", aStatsName);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
				return;
			}
			//if (pSelf->m_apPlayers[StatsID]->m_AccountID < 1)
			//{
			//	str_format(aBuf, sizeof(aBuf), "'%s' is not logged in.", aStatsName);
			//	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			//	return;
			//}

			str_format(aBuf, sizeof(aBuf), "--- %s's Stats ---", aStatsName);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Level[%d]", pSelf->m_apPlayers[StatsID]->m_level);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Xp[%d/%d]", pSelf->m_apPlayers[StatsID]->m_xp, pSelf->m_apPlayers[StatsID]->m_neededxp);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Money[%d]", pSelf->m_apPlayers[StatsID]->m_money);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "PvP-Arena Tickets[%d]", pSelf->m_apPlayers[StatsID]->m_pvp_arena_tickets);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendChatTarget(pResult->m_ClientID, "---- BLOCK ----");
			str_format(aBuf, sizeof(aBuf), "Points: %d", pSelf->m_apPlayers[StatsID]->m_BlockPoints);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Kills: %d", pSelf->m_apPlayers[StatsID]->m_BlockPoints_Kills);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Deaths: %d", pSelf->m_apPlayers[StatsID]->m_BlockPoints_Deaths);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);

		}
		else //own stats
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "--- Your Stats ---");
			str_format(aBuf, sizeof(aBuf), "Level[%d]", pPlayer->m_level);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Xp[%d/%d]", pPlayer->m_xp, pPlayer->m_neededxp);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Money[%d]", pPlayer->m_money);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "PvP-Arena Tickets[%d]", pPlayer->m_pvp_arena_tickets);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendChatTarget(pResult->m_ClientID, "---- BLOCK ----");
			str_format(aBuf, sizeof(aBuf), "Points: %d", pPlayer->m_BlockPoints);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Kills: %d", pPlayer->m_BlockPoints_Kills);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "Deaths: %d", pPlayer->m_BlockPoints_Deaths);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		}
	}
	
}

void CGameContext::ConProfile(IConsole::IResult * pResult, void * pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (pResult->NumArguments() == 0)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "--- Profile help ---");
		pSelf->SendChatTarget(pResult->m_ClientID, "Profiles are connected with your account.");
		pSelf->SendChatTarget(pResult->m_ClientID, "More infos about accounts with '/accountinfo'.");
		pSelf->SendChatTarget(pResult->m_ClientID, "--------------------");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/profile cmdlist' for command list.");
		return;
	}


	char aBuf[128];
	char aPara0[32];
	char aPara1[32];
	str_copy(aPara0, pResult->GetString(0), sizeof(aPara0));
	str_copy(aPara1, pResult->GetString(1), sizeof(aPara1));
	int ViewID = pSelf->GetCIDByName(aPara1);

	if (!str_comp_nocase(aPara0, "help"))
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "--- Profile help ---");
		pSelf->SendChatTarget(pResult->m_ClientID, "Profiles are connected with your account.");
		pSelf->SendChatTarget(pResult->m_ClientID, "More infos about accounts with '/accountinfo'.");
		pSelf->SendChatTarget(pResult->m_ClientID, "--------------------");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/profile cmdlist' for command list.");
	}
	else if (!str_comp_nocase(aPara0, "cmdlist"))
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "--- Profile Commands ---");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/profile view <playername>' to view a players profile.");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/profile style <style>' to change profile style.");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/profile status <status> to change status.'");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/profile email <e-mail>' to change e-mail.");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/profile homepage <homepage>' to change homepage.");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/profile youtube <youtube>' to change youtube.");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/profile skype <skype>' to change skype.");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/profile twitter <twitter>' to change twitter.");
	}
	else if (!str_comp_nocase(aPara0, "view") || !str_comp_nocase(aPara0, "watch"))
	{
		if (pResult->NumArguments() < 2)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Missing parameters. Stick to struct: 'profile view <playername>'.");
			return;
		}

		if (ViewID == -1)
		{
			str_format(aBuf, sizeof(aBuf), "Can't find user: '%s'", aPara1);
			return;
		}

		pSelf->ShowProfile(pResult->m_ClientID, ViewID);
	}
	else if (!str_comp_nocase(aPara0, "style"))
	{



		if (!str_comp_nocase(aPara1, "default"))
		{
			pSelf->m_apPlayers[pResult->m_ClientID]->m_ProfileStyle = 0;
			pSelf->SendChatTarget(pResult->m_ClientID, "Changed profile-style to: default");
		}
		else if (!str_comp_nocase(aPara1, "shit"))
		{
			pSelf->m_apPlayers[pResult->m_ClientID]->m_ProfileStyle = 1;
			pSelf->SendChatTarget(pResult->m_ClientID, "Changed profile-style to: shit");
		}
		else if (!str_comp_nocase(aPara1, "social"))
		{
			pSelf->m_apPlayers[pResult->m_ClientID]->m_ProfileStyle = 2;
			pSelf->SendChatTarget(pResult->m_ClientID, "Changed profile-style to: social");
		}
		else if (!str_comp_nocase(aPara1, "show-off"))
		{
			pSelf->m_apPlayers[pResult->m_ClientID]->m_ProfileStyle = 3;
			pSelf->SendChatTarget(pResult->m_ClientID, "Changed profile-style to: show-off");
		}
		else if (!str_comp_nocase(aPara1, "pvp"))
		{
			pSelf->m_apPlayers[pResult->m_ClientID]->m_ProfileStyle = 4;
			pSelf->SendChatTarget(pResult->m_ClientID, "Changed profile-style to: pvp");
		}
		else if (!str_comp_nocase(aPara1, "bomber"))
		{
			pSelf->m_apPlayers[pResult->m_ClientID]->m_ProfileStyle = 5;
			pSelf->SendChatTarget(pResult->m_ClientID, "Changed profile-style to: bomber");
		}
		else
		{
			str_format(aBuf, sizeof(aBuf), "error: '%s' is not a profile style. Choose between following: default, shit, social, show-off, pvp, bomber", aPara1);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		}
	}
	else if (!str_comp_nocase(aPara0, "status"))
	{
		if (pPlayer->m_AccountID <= 0)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You have to be logged in to use this command.");
			pSelf->SendChatTarget(pResult->m_ClientID, "All infos about accounts: '/accountinfo'");
			return;
		}


		str_copy(pSelf->m_apPlayers[pResult->m_ClientID]->m_ProfileStatus, aPara1, 128);
		str_format(aBuf, sizeof(aBuf), "Updated your profile status: %s", pSelf->m_apPlayers[pResult->m_ClientID]->m_ProfileStatus);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
	else if (!str_comp_nocase(aPara0, "skype"))
	{
		if (pPlayer->m_AccountID <= 0)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You have to be logged in to use this command.");
			pSelf->SendChatTarget(pResult->m_ClientID, "All infos about accounts: '/accountinfo'");
			return;
		}


		str_copy(pSelf->m_apPlayers[pResult->m_ClientID]->m_ProfileSkype, aPara1, 128);
		str_format(aBuf, sizeof(aBuf), "Updated your profile skype: %s", pSelf->m_apPlayers[pResult->m_ClientID]->m_ProfileSkype);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
	else if (!str_comp_nocase(aPara0, "youtube"))
	{
		if (pPlayer->m_AccountID <= 0)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You have to be logged in to use this command.");
			pSelf->SendChatTarget(pResult->m_ClientID, "All infos about accounts: '/accountinfo'");
			return;
		}


		str_copy(pSelf->m_apPlayers[pResult->m_ClientID]->m_ProfileYoutube, aPara1, 128);
		str_format(aBuf, sizeof(aBuf), "Updated your profile youtube: %s", pSelf->m_apPlayers[pResult->m_ClientID]->m_ProfileYoutube);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
	else if (!str_comp_nocase(aPara0, "email") || !str_comp_nocase(aPara0, "e-mail"))
	{
		if (pPlayer->m_AccountID <= 0)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You have to be logged in to use this command.");
			pSelf->SendChatTarget(pResult->m_ClientID, "All infos about accounts: '/accountinfo'");
			return;
		}


		str_copy(pSelf->m_apPlayers[pResult->m_ClientID]->m_ProfileEmail, aPara1, 128);
		str_format(aBuf, sizeof(aBuf), "Updated your profile e-mail: %s", pSelf->m_apPlayers[pResult->m_ClientID]->m_ProfileEmail);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
	else if (!str_comp_nocase(aPara0, "homepage") || !str_comp_nocase(aPara0, "website"))
	{
		if (pPlayer->m_AccountID <= 0)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You have to be logged in to use this command.");
			pSelf->SendChatTarget(pResult->m_ClientID, "All infos about accounts: '/accountinfo'");
			return;
		}


		str_copy(pSelf->m_apPlayers[pResult->m_ClientID]->m_ProfileHomepage, aPara1, 128);
		str_format(aBuf, sizeof(aBuf), "Updated your profile homepage: %s", pSelf->m_apPlayers[pResult->m_ClientID]->m_ProfileHomepage);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
	else if (!str_comp_nocase(aPara0, "homepage"))
	{
		if (pPlayer->m_AccountID <= 0)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You have to be logged in to use this command.");
			pSelf->SendChatTarget(pResult->m_ClientID, "All infos about accounts: '/accountinfo'");
			return;
		}


		str_copy(pSelf->m_apPlayers[pResult->m_ClientID]->m_ProfileTwitter, aPara1, 128);
		str_format(aBuf, sizeof(aBuf), "Updated your profile twitter: %s", pSelf->m_apPlayers[pResult->m_ClientID]->m_ProfileTwitter);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Unknow command. '/profile cmdlist' or '/profile help' might help.");
	}
}

void CGameContext::ConLogin(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	int ClientID = pResult->m_ClientID;
	CPlayer *pPlayer = pSelf->m_apPlayers[ClientID];
	if (!pPlayer)
		return;
	
	if (g_Config.m_SvAccountStuff == 0)
	{
		pSelf->SendChatTarget(ClientID, "account stuff is turned off on this server.");
		return;
	}

	if (pResult->NumArguments() != 2)
	{
		pSelf->SendChatTarget(ClientID, "Please use '/login <name> <password>'.");
		return;
	}

	if (pPlayer->m_AccountID > 0)
	{
		pSelf->SendChatTarget(ClientID, "You are already logged in.");
		return;
	}

	char aUsername[32];
	char aPassword[128];
	str_copy(aUsername, pResult->GetString(0), sizeof(aUsername));
	str_copy(aPassword, pResult->GetString(1), sizeof(aPassword));

	if (str_length(aUsername) > 20 || str_length(aUsername) < 3)
	{
		pSelf->SendChatTarget(ClientID, "Your Username is too long or too short. Max length 20 Min length 3");
		return;
	}

	if (str_length(aPassword) > 20 || str_length(aPassword) < 3)
	{
		pSelf->SendChatTarget(ClientID, "Your Password is too long or too short. Max length 20 Min length 3");
		return;
	}

	char *pQueryBuf = sqlite3_mprintf("SELECT * FROM Accounts WHERE Username='%q' AND Password='%q'", aUsername, aPassword);
	CQueryLogin *pQuery = new CQueryLogin();
	pQuery->m_ClientID = ClientID;
	pQuery->m_pGameServer = pSelf;
	pQuery->Query(pSelf->m_Database, pQueryBuf);
	sqlite3_free(pQueryBuf);
}

void CGameContext::ConChangePassword(IConsole::IResult * pResult, void * pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	if (pPlayer->m_AccountID <= 0)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "You are not logged in. (More info '/accountinfo')");
		return;
	}
	if (pResult->NumArguments() != 3)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Please use: '/changepassword <old> <new> <new_repeate>'");
		return;
	}

	char aOldPass[32];
	char aNewPass[32];
	char aNewPass2[32];
	str_copy(aOldPass, pResult->GetString(0), sizeof(aOldPass));
	str_copy(aNewPass, pResult->GetString(1), sizeof(aNewPass));
	str_copy(aNewPass2, pResult->GetString(2), sizeof(aNewPass2));

	if (str_length(aOldPass) > 20 || str_length(aOldPass) < 3)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Your Oldpassword is too long or too short. Max length 20 Min length 3");
		return;
	}

	if ((str_length(aNewPass) > 20 || str_length(aNewPass) < 3) || (str_length(aNewPass2) > 20 || str_length(aNewPass2) < 3))
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Your Password is too long or too short. Max length 20 Min length 3");
		return;
	}

	if (str_comp_nocase(aNewPass, aNewPass2) != 0)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Passwords have to be the same.");
		return;
	}


	str_format(pPlayer->m_aChangePassword, sizeof(pPlayer->m_aChangePassword), "%s", aNewPass); 
	char *pQueryBuf = sqlite3_mprintf("SELECT * FROM Accounts WHERE Username='%q' AND Password='%q'", pPlayer->m_aAccountLoginName, aOldPass);
	CQueryChangePassword *pQuery = new CQueryChangePassword();
	pQuery->m_ClientID = pResult->m_ClientID;
	pQuery->m_pGameServer = pSelf;
	pQuery->Query(pSelf->m_Database, pQueryBuf);
	sqlite3_free(pQueryBuf);
}

void CGameContext::ConAccLogout(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	int ClientID = pResult->m_ClientID;
	CPlayer *pPlayer = pSelf->m_apPlayers[ClientID];
	if (!pPlayer)
		return;

	if (g_Config.m_SvAccountStuff == 0)
	{
		pSelf->SendChatTarget(ClientID, "account stuff is turned off on this server.");
		return;
	}


	if (pPlayer->m_AccountID <= 0)
	{
		pSelf->SendChatTarget(ClientID, "You are not logged in.");
		return;
	}

	pPlayer->Logout();
	pSelf->SendChatTarget(ClientID, "Logged out.");
}

void CGameContext::ConTogglejailmsg(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	pPlayer->m_hidejailmsg ^= true;
	pSelf->SendBroadcast(" ", pResult->m_ClientID);
}

void CGameContext::ConChidraqul(IConsole::IResult * pResult, void * pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif

	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	char aCommand[64];
	str_copy(aCommand, pResult->GetString(0), sizeof(aCommand));

	if (!str_comp_nocase(aCommand, "help"))
	{
		//send help
	}
	else if (!str_comp_nocase(aCommand, "info"))
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "chidraqul3 more help at '/chidraqul help'");
	}
	else if (!str_comp_nocase(aCommand, "start"))
	{
		if (pPlayer->m_BoughtGame)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "chidraqul started.");
			pPlayer->m_IsMinigame = true;
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You don't have this game. You can buy it with '/buy chidraqul'");
		}
	}
	else if (!str_comp_nocase(aCommand, "stop") || !str_comp_nocase(aCommand, "quit"))
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "chidraqul stopped.");
		pSelf->m_apPlayers[pResult->m_ClientID]->m_IsMinigame = false;
		pSelf->SendBroadcast(" ", pResult->m_ClientID);
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Unknown chidraqul command. try '/chidraqul help'");
	}
}

void CGameContext::ConMinigameLeft(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;


	if (pPlayer->m_IsMinigame)
	{
		if (pPlayer->m_HashPos > 0)
		{
			pPlayer->m_HashPos--;
		}
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "You need to start a minigame first with '/start_minigame' to use the '/Minigameleft' command");
	}

}

void CGameContext::ConMinigameRight(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;


	if (pPlayer->m_IsMinigame)
	{
		if (g_Config.m_SvAllowMinigame == 2)
		{
			if (pPlayer->m_HashPos < 10)
			{
				pPlayer->m_HashPos++;
			}
		}
		else
		{
			if (pPlayer->m_HashPos < pPlayer->m_Minigameworld_size_x - 1)
			{
				pPlayer->m_HashPos++;
			}
		}
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "You need to start a minigame first with '/start_minigame' to use the '/Minigameright' command");
	}

}

void CGameContext::ConMinigameUp(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;


	if (pPlayer->m_IsMinigame)
	{
		if (g_Config.m_SvAllowMinigame == 2)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Admin disableld the /up movement.");
		}
		else
		{
			if (pPlayer->m_HashPosY < 1)
			{
				pPlayer->m_HashPosY++;
			}
		}
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "You need to start a minigame first with '/start_minigame' to use the '/Minigameup' command");
	}

}

void CGameContext::ConMinigameDown(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;


	if (pPlayer->m_IsMinigame)
	{
		if (g_Config.m_SvAllowMinigame == 2)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Admin disabled the /down movement.");
		}
		else
		{
			if (pPlayer->m_HashPosY > 0)
			{
				pPlayer->m_HashPosY--;
			}
		}
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "You need to start a minigame first with '/start_minigame' to use the '/Minigamedown' command");
	}

}

void CGameContext::ConCC(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	if (pPlayer->m_Authed == CServer::AUTHED_ADMIN)
	{

		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'namless rofl' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'(1)namless tee' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'einFISCH' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'GAGAGA' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'Steve-' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'Gro�erDBC' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'cB | Bashcord' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'LIVUS BAGGUGE' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'BoByBANK' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'noobson tnP' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'vali' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'ChiliDreghugn' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'Stahkilla' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'Detztin' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'pikutee <3' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'namless tee' has left the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'BoByBANK' has left the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'Ubu' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'Magnet' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'Jambi*' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'HurricaneZ' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'Sonix' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'darkdragonovernoob' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'Ubu' has left the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'deen' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'fuck me soon' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'fik you!' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'fuckmeson' has left the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'(1)' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'ChilligerDrago' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'(1)ChillerDrago' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'(2)ChillerDrago' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'(3)ChillerDrago' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'Noved' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'Aoe' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'artkis' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'namless brain' entered and joined the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'(1)ChillerDrago' has left the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'HurricaneZ' has left the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'(2)ChillerDrago' has left the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'hax0r' has left the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'(3)ChillerDrago' has left the game");
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "'Destin' has left the game");
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "You don't have enough permissions to use this command.");
	}
}

void CGameContext::ConPvpArena(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	if (pResult->NumArguments() != 1)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Invalid. Type '/pvp_arena ' + 'join' or 'leave'");
		return;
	}


	char aInput[32];
	str_copy(aInput, pResult->GetString(0), 32);

	if (!str_comp_nocase(aInput, "join"))
	{
		if (g_Config.m_SvPvpArenaState)
		{
			if (pPlayer->m_pvp_arena_tickets > 0)
			{
				if (!pPlayer->GetCharacter()->m_IsPVParena)
				{
					pPlayer->m_pvp_arena_tickets--;
					pPlayer->m_pvp_arena_games_played++;
					pPlayer->GetCharacter()->m_IsPVParena = true;
					pPlayer->GetCharacter()->m_isDmg = true;
					pSelf->SendChatTarget(pResult->m_ClientID, "Teleporting to arena... good luck and have fun!");
				}
				else
				{
					pSelf->SendChatTarget(pResult->m_ClientID, "You are already in the PvP-arena");
				}
			}
			else
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "You don't have a ticket. Buy ticket first with '/buy pvp_arena_ticket'");
			}
		}
		else //no arena configurated
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "No pvp-arena found.");
		}
	}
	else if (!str_comp_nocase(aInput, "leave"))
	{
		if (pPlayer->GetCharacter()->m_IsPVParena)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Teleport request sent. Don't move for 6 seconds.");
			pPlayer->GetCharacter()->m_pvp_arena_exit_request_time = pSelf->Server()->TickSpeed() * 6; //6 sekunden
			pPlayer->GetCharacter()->m_pvp_arena_exit_request = true;
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You are not in an arena.");
		}
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Invalid. Type '/pvp_arena ' + 'join' or 'leave'");
	}

}

void CGameContext::ConMoney(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	char aBuf[256];

	pSelf->SendChatTarget(pResult->m_ClientID, "~~~~~~~~~~");
	str_format(aBuf, sizeof(aBuf), "Money: %d", pPlayer->m_money);
	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	pSelf->SendChatTarget(pResult->m_ClientID, "~~~~~~~~~~");
	pSelf->SendChatTarget(pResult->m_ClientID, pPlayer->m_money_transaction0);
	pSelf->SendChatTarget(pResult->m_ClientID, pPlayer->m_money_transaction1);
	pSelf->SendChatTarget(pResult->m_ClientID, pPlayer->m_money_transaction2);
	pSelf->SendChatTarget(pResult->m_ClientID, pPlayer->m_money_transaction3);
	pSelf->SendChatTarget(pResult->m_ClientID, pPlayer->m_money_transaction4);
	pSelf->SendChatTarget(pResult->m_ClientID, pPlayer->m_money_transaction5);
	pSelf->SendChatTarget(pResult->m_ClientID, pPlayer->m_money_transaction6);
	pSelf->SendChatTarget(pResult->m_ClientID, pPlayer->m_money_transaction7);
	pSelf->SendChatTarget(pResult->m_ClientID, pPlayer->m_money_transaction8);
	pSelf->SendChatTarget(pResult->m_ClientID, pPlayer->m_money_transaction9);
	pSelf->SendChatTarget(pResult->m_ClientID, "~~~~~~~~~~");
}

void CGameContext::ConPay(IConsole::IResult * pResult, void * pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	if (pResult->NumArguments() != 2)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "use '/pay <amount> <player>' to send other players your '/money'");
		return;
	}


	char aBuf[512];
	int Amount;
	char aUsername[32];
	Amount = pResult->GetInteger(0);
	str_copy(aUsername, pResult->GetString(1), sizeof(aUsername));
	int PayID = pSelf->GetCIDByName(aUsername);


	//COUDL DO:
	// add a blocker to pay money to ur self... but me funny mede it pozzible


	if (Amount > pPlayer->m_money)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "you don't have that much money mate -.-");
		return;
	}

	if (Amount < 0)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "lel are u triin' to steal money?");
		return;
	}

	if (Amount == 0)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "you paid nothing.");
		return;
	}

	if (PayID == -1)
	{
		str_format(aBuf, sizeof(aBuf), "Can't find a user with the name: %s", aUsername);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
	else
	{
		if (pSelf->m_apPlayers[PayID]->m_AccountID < 1)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "ERROR: This player is not logged in. More info '/accountinfo'");
			return;
		}


		//player give
		str_format(aBuf, sizeof(aBuf), "You paid %d money to the player '%s'", Amount, aUsername);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		str_format(aBuf, sizeof(aBuf), "-%d paid to '%s'", Amount, aUsername);
		pPlayer->MoneyTransaction(-Amount, aBuf);

		//player get
		str_format(aBuf, sizeof(aBuf), "'%s' paid you %d money", Amount, pSelf->Server()->ClientName(pResult->m_ClientID));
		pSelf->SendChatTarget(PayID, aBuf);
		str_format(aBuf, sizeof(aBuf), "+%d paid by '%s'", Amount, pSelf->Server()->ClientName(pResult->m_ClientID));
		pSelf->m_apPlayers[PayID]->MoneyTransaction(Amount, aBuf);
	}

}

void CGameContext::ConGift(IConsole::IResult * pResult, void * pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	char aBuf[256];

	if (pResult->NumArguments() == 0)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "**** GIFT INFO ****");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/gift <player>' to send someone 150 money.");
		pSelf->SendChatTarget(pResult->m_ClientID, "You don't loose this money. It is coming from the server.");
		pSelf->SendChatTarget(pResult->m_ClientID, "**** GIFT STATUS ****");
		if (pPlayer->m_GiftDelay)
		{
			str_format(aBuf, sizeof(aBuf), "You can send gifts in %d seconds.", pPlayer->m_GiftDelay / pSelf->Server()->TickSpeed());
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You can send gifts.");
		}
		return;
	}

	int GiftID = pSelf->GetCIDByName(pResult->GetString(0));

	if (GiftID == -1)
	{
		str_format(aBuf, sizeof(aBuf), "player '%s' is not online.", pResult->GetString(0));
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		return;
	}
	if (pPlayer->m_GiftDelay)
	{
		str_format(aBuf, sizeof(aBuf), "You can send gifts in %d seconds.", pPlayer->m_GiftDelay / pSelf->Server()->TickSpeed());
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		return;
	}
	if (pPlayer->m_level < 1)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "You have to be atleast level 1 to use gifts.");
		return;
	}

	if (pSelf->m_apPlayers[GiftID])
	{
		char aOwnIP[128];
		char aGiftIP[128];
		pSelf->Server()->GetClientAddr(pResult->m_ClientID, aOwnIP, sizeof(aOwnIP));
		pSelf->Server()->GetClientAddr(GiftID, aGiftIP, sizeof(aGiftIP));

		if (!str_comp_nocase(aOwnIP, aGiftIP))
			pSelf->SendChatTarget(pResult->m_ClientID, "You can't give money to your dummy.");
		else
		{
			pSelf->m_apPlayers[GiftID]->MoneyTransaction(+150, "+150 gift");
			str_format(aBuf, sizeof(aBuf), "You gave %s 150 money!", pSelf->Server()->ClientName(GiftID));
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);

			str_format(aBuf, sizeof(aBuf), "%s has gifted you +150 money. (more info '/gift')", pSelf->Server()->ClientName(pResult->m_ClientID));
			pSelf->SendChatTarget(GiftID, aBuf);


			pSelf->m_apPlayers[pResult->m_ClientID]->m_GiftDelay = pSelf->Server()->TickSpeed() * 180; //180 seconds == 3 minutes
		}
	}
}

void CGameContext::ConEvent(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	pSelf->SendChatTarget(pResult->m_ClientID, "###########################");
	if (g_Config.m_SvFinishEvent == 1)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "~~~ Race Event ~~~");
		pSelf->SendChatTarget(pResult->m_ClientID, "Info: You get more xp for finishing the map!");
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "no events running at the moment...");
	}

	pSelf->SendChatTarget(pResult->m_ClientID, "###########################");
}


// accept/turn-off cosmetic features

void CGameContext::ConRainbow(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	if (pResult->NumArguments() != 1)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Invalid. Type '/rainbow ' + 'accept' or 'off'");
		return;
	}


	char aInput[32];
	str_copy(aInput, pResult->GetString(0), 32);

	if (!str_comp_nocase(aInput, "off"))
	{
		pPlayer->GetCharacter()->m_Rainbow = false;
		pPlayer->m_InfRainbow = false;
		pSelf->SendChatTarget(pResult->m_ClientID, "Rainbow turned off");
	}
	else if (!str_comp_nocase(aInput, "accept"))
	{
		if (pPlayer->m_rainbow_offer > 0)
		{
			if (!pPlayer->GetCharacter()->m_Rainbow)
			{
				pPlayer->GetCharacter()->m_Rainbow = true;
				pPlayer->m_rainbow_offer--;
				pSelf->SendChatTarget(pResult->m_ClientID, "You accepted rainbow. You can turn off rainbow with '/rainbow off'");
			}
			else
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "You already have rainbow.");
			}
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Nobody offered you rainbow.");
		}
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Invalid. Type '/rainbow ' + 'accept' or 'off'");
	}
}

void CGameContext::ConBloody(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	if (pResult->NumArguments() != 1)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Invalid. Type '/bloody ' + 'accept' or 'off'");
		return;
	}


	char aInput[32];
	str_copy(aInput, pResult->GetString(0), 32);

	if (!str_comp_nocase(aInput, "off"))
	{
			pPlayer->GetCharacter()->m_Bloody = false;
			pPlayer->m_InfBloody = false;
			pSelf->SendChatTarget(pResult->m_ClientID, "bloody turned off");
	}
	else if (!str_comp_nocase(aInput, "accept"))
	{
		if (pPlayer->m_bloody_offer > 0)
		{
			if (!pPlayer->GetCharacter()->m_Bloody)
			{
				pPlayer->GetCharacter()->m_Bloody = true;
				pPlayer->m_bloody_offer--;
				pSelf->SendChatTarget(pResult->m_ClientID, "You accepted bloody. You can turn off bloody with '/bloody off'");
			}
			else
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "You already have bloody.");
			}
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Nobody offered you bloody.");
		}
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Invalid. Type '/bloody ' + 'accept' or 'off'");
	}
}

void CGameContext::ConAtom(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	if (pResult->NumArguments() != 1)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Invalid. Type '/atom ' + 'accept' or 'off'");
		return;
	}


	char aInput[32];
	str_copy(aInput, pResult->GetString(0), 32);

	if (!str_comp_nocase(aInput, "off"))
	{
			pPlayer->GetCharacter()->m_Atom = false;
			pPlayer->m_InfAtom = false;
			pSelf->SendChatTarget(pResult->m_ClientID, "atom turned off");
	}
	else if (!str_comp_nocase(aInput, "accept"))
	{
		if (pPlayer->m_atom_offer > 0)
		{
			if (!pPlayer->GetCharacter()->m_Atom)
			{
				pPlayer->GetCharacter()->m_Atom = true;
				pPlayer->m_atom_offer--;
				pSelf->SendChatTarget(pResult->m_ClientID, "You accepted atom. You can turn off atom with '/atom off'");
			}
			else
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "You already have atom.");
			}
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Nobody offered you atom.");
		}
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Invalid. Type '/atom ' + 'accept' or 'off'");
	}
}

void CGameContext::ConTrail(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	if (pResult->NumArguments() != 1)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Invalid. Type '/trail ' + 'accept' or 'off'");
		return;
	}


	char aInput[32];
	str_copy(aInput, pResult->GetString(0), 32);

	if (!str_comp_nocase(aInput, "off"))
	{
			pPlayer->GetCharacter()->m_Trail = false;
			pPlayer->m_InfTrail = false;
			pSelf->SendChatTarget(pResult->m_ClientID, "trail turned off");
	}
	else if (!str_comp_nocase(aInput, "accept"))
	{
		if (pPlayer->m_trail_offer > 0)
		{
			if (!pPlayer->GetCharacter()->m_Trail)
			{
				pPlayer->GetCharacter()->m_Trail = true;
				pPlayer->m_trail_offer--;
				pSelf->SendChatTarget(pResult->m_ClientID, "You accepted trail. You can turn off trail with '/trail off'");
			}
			else
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "You already have trail.");
			}
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Nobody offered you trail.");
		}
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Invalid. Type '/trail ' + 'accept' or 'off'");
	}
}

void CGameContext::ConAccountInfo(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;


	pSelf->SendChatTarget(pResult->m_ClientID, "~~~ Account Info ~~~");
	pSelf->SendChatTarget(pResult->m_ClientID, "How to register?");
	pSelf->SendChatTarget(pResult->m_ClientID, "/register <name> <password> <password>");
	pSelf->SendChatTarget(pResult->m_ClientID, "How to login?");
	pSelf->SendChatTarget(pResult->m_ClientID, "/login <name> <password>");
	//pSelf->SendChatTarget(pResult->m_ClientID, " ");
	//pSelf->SendChatTarget(pResult->m_ClientID, "Tipp: name and password shoudl be different");
}

void CGameContext::ConPoliceInfo(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;


	pSelf->SendChatTarget(pResult->m_ClientID, "~~~ Policeinfo ~~~");
	pSelf->SendChatTarget(pResult->m_ClientID, "What are the police benefits?");
	pSelf->SendChatTarget(pResult->m_ClientID, "The police can write in '/policechat' to get extra attention from players, announce policehelpers, get extra money per policerank from money-tiles and has a policetaser. ");
	pSelf->SendChatTarget(pResult->m_ClientID, "How many policeranks are there?");
	pSelf->SendChatTarget(pResult->m_ClientID, "Currently there are 3 policeranks, you get +1 extra money for each rank.");
	pSelf->SendChatTarget(pResult->m_ClientID, "How to become police?");
	pSelf->SendChatTarget(pResult->m_ClientID, "When you hit level 18, you can buy the first policerank in '/shop'");
	pSelf->SendChatTarget(pResult->m_ClientID, "For more information about the policetaser, type '/taserinfo'");

}


//void CGameContext::ConProfileInfo(IConsole::IResult *pResult, void *pUserData) //old
//{
//#if defined(CONF_DEBUG)
//	CALL_STACK_ADD();
//#endif
//	CGameContext *pSelf = (CGameContext *)pUserData;
//	if (!CheckClientID(pResult->m_ClientID))
//		return;
//
//	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
//	if (!pPlayer)
//		return;
//
//	CCharacter* pChr = pPlayer->GetCharacter();
//	if (!pChr)
//		return;
//
//
//	pSelf->SendChatTarget(pResult->m_ClientID, "~~~ Profile Info ~~~");
//	pSelf->SendChatTarget(pResult->m_ClientID, " ");
//	pSelf->SendChatTarget(pResult->m_ClientID, "VIEW PROFILES:");
//	pSelf->SendChatTarget(pResult->m_ClientID, "/profile view (playername)");
//	pSelf->SendChatTarget(pResult->m_ClientID, "Info: The player needs to be on the server and logged in");
//	pSelf->SendChatTarget(pResult->m_ClientID, " ");
//	pSelf->SendChatTarget(pResult->m_ClientID, "PROFILE SETTINGS:");
//	pSelf->SendChatTarget(pResult->m_ClientID, "/profile_style (style) - changes your profile style");
//	pSelf->SendChatTarget(pResult->m_ClientID, "/profile_status (status) - changes your status");
//	pSelf->SendChatTarget(pResult->m_ClientID, "/profile_skype (skype) - changes your skype");
//	pSelf->SendChatTarget(pResult->m_ClientID, "/profile_youtube (youtube) - changes your youtube");
//	pSelf->SendChatTarget(pResult->m_ClientID, "/profile_email (email) - changes your email");
//	pSelf->SendChatTarget(pResult->m_ClientID, "/profile_homepage (homepage) - changes your homepage");
//	pSelf->SendChatTarget(pResult->m_ClientID, "/profile_twitter (twitter) - changes your twitter");
//}

void CGameContext::ConTCMD3000(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	char aBuf[128];

	str_format(aBuf, sizeof(aBuf), "Cucumber value: %d", pSelf->m_CucumberShareValue);
	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
}

void CGameContext::ConStockMarket(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	char aBuf[256];
	char aInput[32];
	str_copy(aInput, pResult->GetString(0), 32);

	if (!str_comp_nocase(aInput, "buy"))
	{
		if (pPlayer->m_money < pSelf->m_CucumberShareValue)
		{
			str_format(aBuf, sizeof(aBuf), "You don't have enough money. You need %d money.", pSelf->m_CucumberShareValue);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		}
		else
		{
			pPlayer->m_StockMarket_item_Cucumbers++;

			str_format(aBuf, sizeof(aBuf), "-%d bought a cucumber stock", pSelf->m_CucumberShareValue);
			pPlayer->MoneyTransaction(-pSelf->m_CucumberShareValue, aBuf);


			pSelf->m_CucumberShareValue++; //push the gernerall share value
		}

	}
	else if (!str_comp_nocase(aInput, "sell"))
	{
		if (pPlayer->m_StockMarket_item_Cucumbers > 0)
		{
			pPlayer->m_StockMarket_item_Cucumbers--;


			str_format(aBuf, sizeof(aBuf), "+%d sold a cucumber stock", pSelf->m_CucumberShareValue);
			pPlayer->MoneyTransaction(+pSelf->m_CucumberShareValue, aBuf);

			pSelf->m_CucumberShareValue--; //pull the gernerall share value
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "you dont have this stock");
		}
	}
	else if (!str_comp_nocase(aInput, "info"))
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "==== PUBLIC STOCK MARKET ====");
		str_format(aBuf, sizeof(aBuf), "Cucumbers %d money", pSelf->m_CucumberShareValue);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		pSelf->SendChatTarget(pResult->m_ClientID, "==== PERSONAL STATS ====");
		str_format(aBuf, sizeof(aBuf), "Cucumbers %d", pPlayer->m_StockMarket_item_Cucumbers);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "error. Type '/StockMarket ' + 'sell' or 'buy' or 'info'");
	}
}

void CGameContext::ConPoop(IConsole::IResult * pResult, void * pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	if (pResult->NumArguments() != 2)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Throw shit at other players. Warning: you loose that shit.");
		pSelf->SendChatTarget(pResult->m_ClientID, "use '/poop <amount> <player>'");
		return;
	}


	char aBuf[512];
	int Amount;
	char aUsername[32];
	Amount = pResult->GetInteger(0);
	str_copy(aUsername, pResult->GetString(1), sizeof(aUsername));
	int PoopID = pSelf->GetCIDByName(aUsername);


	//COUDL DO:
	// add a blocker to poop ur self... but me funny mede it pozzible


	if (Amount > pPlayer->m_shit)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "you don't have enough shit.");
		return;
	}

	if (Amount < 0)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "you can't poop negative?! Imagine some1 is tring to push shit back in ur anus ... wtf");
		return;
	}
	if (Amount == 0)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "you don't have shit");
		return;
	}

	if (PoopID == -1)
	{
		str_format(aBuf, sizeof(aBuf), "Can't find a user with the name: %s", aUsername);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
	else
	{
		if (pSelf->m_apPlayers[PoopID]->m_AccountID < 1)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "ERROR: This player is not logged in. More info '/accountinfo'");
			return;
		}


		//player give
		str_format(aBuf, sizeof(aBuf), "You pooped %s %d times xd", aUsername, Amount);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		str_format(aBuf, sizeof(aBuf), "you lost %d shit ._.", Amount);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		pPlayer->m_shit -= Amount;

		//player get
		if (g_Config.m_SvPoopMSG == 1) //normal
		{
			str_format(aBuf, sizeof(aBuf), "%s threw %d shit at you o.O", aUsername, Amount);
			pSelf->SendChatTarget(PoopID, aBuf);
		}
		else if (g_Config.m_SvPoopMSG == 2) //extreme
		{
			for (int i = 0; i < Amount; i++)
			{
				str_format(aBuf, sizeof(aBuf), "%s threw shit at you o.O", aUsername);
				pSelf->SendChatTarget(PoopID, aBuf);

				if (i > 30) //poop blocker o.O 30 lines of poop is the whole chat. Poor server has enough
				{
					str_format(aBuf, sizeof(aBuf), "%s threw %d shit at you o.O", aUsername, Amount); //because it was more than the chatwindow can show inform the user how much poop it was
					pSelf->SendChatTarget(PoopID, aBuf);
					break;
				}
			}
		}
		pSelf->m_apPlayers[PoopID]->m_shit += Amount;
	}

}


void CGameContext::ConGive(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;


	if (pResult->NumArguments() == 0)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "~~~ GIVE COMMAND ~~~");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/give <extra>' to give it your self");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/give <extra> <player>' to give it <player>");
		pSelf->SendChatTarget(pResult->m_ClientID, "-- EXTRAS --");
		pSelf->SendChatTarget(pResult->m_ClientID, "rainbow, bloody, trail, atom");
		return;
	}


	char aBuf[512];


	//Ranks sorted DESC by power
	//---> the highest rank gets triggerd

	//the ASC problem is if a SuperModerator is also rcon_mod he only has rcon_mod powerZ



	//COUDL DO:
	//Im unsure to check if GiveID is logged in. 
	//Pros:
	//- moderators can make random players happy and they dont have to spend time to login
	//Cons:
	//- missing motivation to create an account


	if (pPlayer->m_Authed == CServer::AUTHED_ADMIN)
	{
		if (pResult->NumArguments() == 1) //only item no player --> give it ur self
		{
			char aItem[64];
			str_copy(aItem, pResult->GetString(0), sizeof(aItem));
			if (!str_comp_nocase(aItem, "bloody"))
			{
				pPlayer->GetCharacter()->m_Bloody = true;
				pSelf->SendChatTarget(pResult->m_ClientID, "Bloody on.");
			}
			else if (!str_comp_nocase(aItem, "rainbow"))
			{
				pPlayer->GetCharacter()->m_Rainbow = true;
				pSelf->SendChatTarget(pResult->m_ClientID, "Rainbow on.");
			}
			else if (!str_comp_nocase(aItem, "trail"))
			{
				pPlayer->GetCharacter()->m_Trail = true;
				pSelf->SendChatTarget(pResult->m_ClientID, "Trail on.");
			}
			else if (!str_comp_nocase(aItem, "atom"))
			{
				pPlayer->GetCharacter()->m_Atom = true;
				pSelf->SendChatTarget(pResult->m_ClientID, "Atom on.");
			}
			else
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Unknown item.");
			}
		}
		else if (pResult->NumArguments() == 2) //give to other players
		{
			char aItem[64];
			char aUsername[32];
			str_copy(aItem, pResult->GetString(0), sizeof(aItem));
			str_copy(aUsername, pResult->GetString(1), sizeof(aUsername));

			int GiveID = -1;
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (pSelf->m_apPlayers[i])
				{
					if (!str_comp(pSelf->Server()->ClientName(i), aUsername))
					{
						GiveID = i;
						break;
					}
				}
			}

			if (GiveID != -1)
			{
				if (!str_comp_nocase(aItem, "bloody"))
				{
					if (pSelf->m_apPlayers[GiveID]->m_bloody_offer > 4)
					{
						pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission. Admins can't give more than 5 bloody offer to the same player.");
					}
					else
					{
						pSelf->m_apPlayers[GiveID]->m_bloody_offer++;
						str_format(aBuf, sizeof(aBuf), "Bloody offer given to the user: %s", aUsername);
						pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
					}
				}
				else if (!str_comp_nocase(aItem, "rainbow"))
				{
					if (pSelf->m_apPlayers[GiveID]->m_rainbow_offer > 19)
					{
						pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission. Admins can't give more than 20 rainbow offers to the same player.");
					}
					else
					{
						pSelf->m_apPlayers[GiveID]->m_rainbow_offer++;
						str_format(aBuf, sizeof(aBuf), "Rainbow offer given to the user: %s", aUsername);
						pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
					}
				}
				else if (!str_comp_nocase(aItem, "trail"))
				{
					if (pSelf->m_apPlayers[GiveID]->m_trail_offer > 9)
					{
						pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission. Admins can't give more than 10 trail offers to the same player.");
						return;
					}

					pSelf->m_apPlayers[GiveID]->m_trail_offer++;
					str_format(aBuf, sizeof(aBuf), "Trail offer given to the user: %s", aUsername);
					pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
				}
				else if (!str_comp_nocase(aItem, "atom"))
				{
					if (pSelf->m_apPlayers[GiveID]->m_atom_offer > 9)
					{
						pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission. Admins can't give more than 10 atom offers to the same player.");
						return;
					}

					pSelf->m_apPlayers[GiveID]->m_atom_offer++;
					str_format(aBuf, sizeof(aBuf), "Atom offer given to the user: %s", aUsername);
					pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
				}
				else
				{
					pSelf->SendChatTarget(pResult->m_ClientID, "Unknown item.");
				}
			}
			else
			{
				str_format(aBuf, sizeof(aBuf), "Can't find a user with the name: %s", aUsername);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			}
		}
	}
	else if (pPlayer->m_IsSuperModerator)
	{
		if (pResult->NumArguments() == 1) //only item no player --> give it ur self
		{
			char aItem[64];
			str_copy(aItem, pResult->GetString(0), sizeof(aItem));
			if (!str_comp_nocase(aItem, "bloody"))
			{
				pPlayer->GetCharacter()->m_Bloody = true;
				pSelf->SendChatTarget(pResult->m_ClientID, "Bloody on.");
			}
			else if (!str_comp_nocase(aItem, "rainbow"))
			{
				pPlayer->GetCharacter()->m_Rainbow = true;
				pSelf->SendChatTarget(pResult->m_ClientID, "Rainbow on.");
			}
			else if (!str_comp_nocase(aItem, "trail"))
			{
				pPlayer->GetCharacter()->m_Trail = true;
				pSelf->SendChatTarget(pResult->m_ClientID, "Trail on.");
			}
			else if (!str_comp_nocase(aItem, "atom"))
			{
				pPlayer->GetCharacter()->m_Atom = true;
				pSelf->SendChatTarget(pResult->m_ClientID, "Atom on.");
			}
			else
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Unknown item.");
			}
		}
		else if (pResult->NumArguments() == 2) //give to other players
		{
			char aItem[64];
			char aUsername[32];
			str_copy(aItem, pResult->GetString(0), sizeof(aItem));
			str_copy(aUsername, pResult->GetString(1), sizeof(aUsername));

			int GiveID = -1;
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (pSelf->m_apPlayers[i])
				{
					if (!str_comp(pSelf->Server()->ClientName(i), aUsername))
					{
						GiveID = i;
						break;
					}
				}
			}

			if (GiveID != -1)
			{
				if (!str_comp_nocase(aItem, "bloody"))
				{
					if (pSelf->m_apPlayers[GiveID]->m_bloody_offer)
					{
						pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission. SuperModerators can't give more than one bloody offer to the same player.");
					}
					else
					{
						pSelf->m_apPlayers[GiveID]->m_bloody_offer++;
						str_format(aBuf, sizeof(aBuf), "Bloody offer given to the user: %s", aUsername);
						pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
					}
				}
				else if (!str_comp_nocase(aItem, "rainbow"))
				{
					if (pSelf->m_apPlayers[GiveID]->m_rainbow_offer > 9)
					{
						pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission. SuperModerators can't give more than 10 rainbow offers to the same player.");
					}
					else
					{
						pSelf->m_apPlayers[GiveID]->m_rainbow_offer++;
						str_format(aBuf, sizeof(aBuf), "Rainbow offer given to the user: %s", aUsername);
						pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
					}
				}
				else if (!str_comp_nocase(aItem, "trail"))
				{
					pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission.");
				}
				else if (!str_comp_nocase(aItem, "atom"))
				{
					pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission.");
				}
				else
				{
					pSelf->SendChatTarget(pResult->m_ClientID, "Unknown item.");
				}
			}
			else
			{
				str_format(aBuf, sizeof(aBuf), "Can't find a user with the name: %s", aUsername);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			}
		}
	}
	else if (pPlayer->m_IsModerator)
	{
		if (pResult->NumArguments() == 1) //only item no player --> give it ur self
		{
			char aItem[64];
			str_copy(aItem, pResult->GetString(0), sizeof(aItem));
			if (!str_comp_nocase(aItem, "bloody"))
			{
				pPlayer->GetCharacter()->m_Bloody = true;
				pSelf->SendChatTarget(pResult->m_ClientID, "Bloody on.");
			}
			else if (!str_comp_nocase(aItem, "rainbow"))
			{
				pPlayer->GetCharacter()->m_Rainbow = true;
				pSelf->SendChatTarget(pResult->m_ClientID, "Rainbow on.");
			}
			else if (!str_comp_nocase(aItem, "trail"))
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission.");
			}
			else if (!str_comp_nocase(aItem, "atom"))
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission.");
			}
			else
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Unknown item.");
			}
		}
		else if (pResult->NumArguments() == 2) //give to other players
		{
			char aItem[64];
			char aUsername[32];
			str_copy(aItem, pResult->GetString(0), sizeof(aItem));
			str_copy(aUsername, pResult->GetString(1), sizeof(aUsername));

			int GiveID = -1;
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (pSelf->m_apPlayers[i])
				{
					if (!str_comp(pSelf->Server()->ClientName(i), aUsername))
					{
						GiveID = i;
						break;
					}
				}
			}

			if (GiveID != -1)
			{
				if (!str_comp_nocase(aItem, "bloody"))
				{
					pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission.");
				}
				else if (!str_comp_nocase(aItem, "rainbow"))
				{
					if (pSelf->m_apPlayers[GiveID]->m_rainbow_offer)
					{
						pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission. Moderators can't give more than one offer to the same player.");
					}
					else
					{
						pSelf->m_apPlayers[GiveID]->m_rainbow_offer++;
						str_format(aBuf, sizeof(aBuf), "Rainbow offer given to the user: %s", aUsername);
						pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
					}
				}
				else if (!str_comp_nocase(aItem, "trail"))
				{
					pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission.");
				}
				else if (!str_comp_nocase(aItem, "atom"))
				{
					pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission.");
				}
				else
				{
					pSelf->SendChatTarget(pResult->m_ClientID, "Unknown item.");
				}
			}
			else
			{
				str_format(aBuf, sizeof(aBuf), "Can't find a user with the name: %s", aUsername);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			}
		}
	}
	else if (pPlayer->m_Authed == CServer::AUTHED_MOD)
	{
		char aItem[64];
		str_copy(aItem, pResult->GetString(0), sizeof(aItem));
		if (!str_comp_nocase(aItem, "bloody"))
		{
			pPlayer->GetCharacter()->m_Bloody = true;
			pSelf->SendChatTarget(pResult->m_ClientID, "Bloody on.");
		}
		else if (!str_comp_nocase(aItem, "rainbow"))
		{
			pPlayer->GetCharacter()->m_Rainbow = true;
			pSelf->SendChatTarget(pResult->m_ClientID, "Rainbow on.");
		}
		else if (!str_comp_nocase(aItem, "trail"))
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission.");
		}
		else if (!str_comp_nocase(aItem, "atom"))
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission.");
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Unknown item.");
		}
	}
	else //no rank at all
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission.");
	}
}

void CGameContext::ConBomb(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif

	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	if (!g_Config.m_SvAllowBomb)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Bomb games are deactivated by an admin.");
		return;
	}

	if (pResult->NumArguments() == 0)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Missing parameter. Check '/bomb help' for more help.");
		return;
	}


	char aBuf[512];

	char aCmd[64];

	str_copy(aCmd, pResult->GetString(0), sizeof(aCmd));

	if (!str_comp_nocase(aCmd, "create"))
	{
		if (pResult->NumArguments() < 2)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "ERROR: missing money parameter ('/bomb create <money>')");
			return;
		}
		if (pSelf->m_BombGameState)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "ERROR: there is already a bomb game. You can join it with '/bomb join'");
			return;
		}
		if (pPlayer->m_AccountID < 1)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You need to be logged in to create a bomb game. More info at '/accountinfo'");
			return;
		}
		if (pPlayer->m_BombBanTime)
		{
			str_format(aBuf, sizeof(aBuf), "You are banned for %d seconds from bomb games.", pPlayer->m_BombBanTime / 60);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			return;
		}

		int BombMoney;
		BombMoney = pResult->GetInteger(1);

		if (BombMoney > pPlayer->m_money)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "ERROR: you don't have enough money.");
			return;
		}
		if (BombMoney < 0)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "ERROR: bomb reward has to be positive.");
			return;
		}

		if (pResult->NumArguments() > 2) //config map
		{
			char aConfig[32];
			str_copy(aConfig, pResult->GetString(2), sizeof(aConfig));

			if (!str_comp_nocase(aConfig, "NoArena"))
			{
				str_format(pSelf->m_BombMap, sizeof(pSelf->m_BombMap), "NoArena");
			}
			else if (!str_comp_nocase(aConfig, "Default"))
			{
				str_format(pSelf->m_BombMap, sizeof(pSelf->m_BombMap), "Default");
			}
			else
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "--------[BOMB]--------");
				pSelf->SendChatTarget(pResult->m_ClientID, "ERROR: unknown map. Aviable maps: ");
				pSelf->SendChatTarget(pResult->m_ClientID, "Default");
				pSelf->SendChatTarget(pResult->m_ClientID, "NoArena");
				pSelf->SendChatTarget(pResult->m_ClientID, "----------------------");
				return;
			}
		}
		else //no config --> Default
		{
			str_format(pSelf->m_BombMap, sizeof(pSelf->m_BombMap), "Default");
		}

		pSelf->m_apPlayers[pResult->m_ClientID]->m_BombTicksUnready = 0;
		pSelf->m_BombMoney = BombMoney;
		pSelf->m_BombGameState = 1;
		pChr->m_IsBombing = true;
		str_format(aBuf, sizeof(aBuf), "-%d bomb (join)", BombMoney);
		pPlayer->MoneyTransaction(-BombMoney, aBuf);

		str_format(aBuf, sizeof(aBuf), "[BOMB] You have created a game lobby. Map: %s", pSelf->m_BombMap);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		str_format(aBuf, sizeof(aBuf), " -%d money for joining this bomb game.", BombMoney);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
	else if (!str_comp_nocase(aCmd, "join"))
	{
		if (pPlayer->m_AccountID < 1)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You need to be logged in to join a bomb game. More info at '/accountinfo'");
			return;
		}
		if (pPlayer->m_BombBanTime)
		{
			str_format(aBuf, sizeof(aBuf), "You are banned for %d seconds from bomb games.", pPlayer->m_BombBanTime / 60);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			return;
		}
		if (pChr->m_IsBombing)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You are already in the bomb game.");
			return;
		}

		if (!pSelf->m_BombGameState)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "No bomb game running. You can create a new one with '/bomb create <money>'");
			return;
		}
		else if (pSelf->m_BombGameState == 2)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "The bomb lobby is locked.");
			return;
		}
		else if (pSelf->m_BombGameState == 3)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "The bomb game is already running.");
			return;
		}
		else if (pSelf->m_BombGameState == 1)
		{
			if (pPlayer->m_money < pSelf->m_BombMoney)
			{
				str_format(aBuf, sizeof(aBuf), "You need at least %d money to join this game.", pSelf->m_BombMoney);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
				return;
			}

			str_format(aBuf, sizeof(aBuf), "-%d money for joining this game. You don't want to risk that much money? -> '/bomb leave'", pSelf->m_BombMoney);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendChatTarget(pResult->m_ClientID, "You will die in this game! So better leave if you want to keep weapons and stuff.");
			pChr->m_IsBombing = true;
			str_format(aBuf, sizeof(aBuf), "-%d bomb (join)", pSelf->m_BombMoney);
			pPlayer->MoneyTransaction(-pSelf->m_BombMoney, aBuf);
			pPlayer->m_BombTicksUnready = 0;
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Something went horrible wrong lol. Please contact an admin.");
			return;
		}
	}
	else if (!str_comp_nocase(aCmd, "leave"))
	{
		if (!pChr->m_IsBombing)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You are not in a bomb game.");
			return;
		}
		if (pChr->m_IsBombing && pSelf->m_BombGameState == 3)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You can't leave a running game. (If you disconnect, your money will be lost.)");
			return;
		}

		str_format(aBuf, sizeof(aBuf), "You left the bomb game. (+%d money)", pSelf->m_BombMoney);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		str_format(aBuf, sizeof(aBuf), "+%d bomb (leave)", pSelf->m_BombMoney);
		pPlayer->MoneyTransaction(pSelf->m_BombMoney, aBuf);
		pSelf->SendBroadcast("", pResult->m_ClientID);
		pChr->m_IsBombing = false;
		pChr->m_IsBomb = false;
		pChr->m_IsBombReady = false;
	}
	else if (!str_comp_nocase(aCmd, "start"))
	{
		if (!pChr->m_IsBombing)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You are not in a bomb game. Type '/bomb create <money>' or '/bomb join' first.");
			return;
		}
		if (pChr->m_IsBombReady)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You are already ready to start. (If you aren't ready anymore try '/bomb leave')");
			return;
		}
		if (pChr->m_IsBombing && pSelf->m_BombGameState == 3) //shoudl be never triggerd but yolo xd
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Game is already running...");
			return;
		}

		pSelf->SendChatTarget(pResult->m_ClientID, "You are now ready to play. Waiting for other players...");
		pChr->m_IsBombReady = true;
	}
	else if (!str_comp_nocase(aCmd, "lock")) 
	{
		if (!pChr->m_IsBombing)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You are not in a bomb game.");
			return;
		}
		if (pChr->m_IsBombing && pSelf->m_BombGameState == 3) 
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Running games are locked automatically.");
			return;
		}
		if (pSelf->CountBombPlayers() == 1)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "More bomb players needed to lock the lobby.");
			return;
		}
		if (g_Config.m_SvBombLockable == 0) //off
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Locking bomblobbys is deactivated on this server.");
			return;
		}
		else if (g_Config.m_SvBombLockable == 1) //mods and higher
		{
			if (!pPlayer->m_Authed)
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Only rcon authed players can lock bomb games.");
				return;
			}
		}

		if (pSelf->m_BombGameState == 1) //unlocked --> lock
		{
			//lock it
			pSelf->m_BombGameState = 2;

			//send lock message to all bombers
			str_format(aBuf, sizeof(aBuf), "'%s' locked the bomb lobby.", pSelf->Server()->ClientName(pResult->m_ClientID));
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (pSelf->GetPlayerChar(i))
				{
					if (pSelf->GetPlayerChar(i)->m_IsBombing)
					{
						pSelf->SendChatTarget(i, aBuf);
					}
				}
			}
		}
		else if (pSelf->m_BombGameState == 2) //locked --> unlock
		{
			//unlock it
			pSelf->m_BombGameState = 1;

			//send unlock message to all bombers
			str_format(aBuf, sizeof(aBuf), "'%s' unlocked the bomb lobby.", pSelf->Server()->ClientName(pResult->m_ClientID));
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (pSelf->GetPlayerChar(i))
				{
					if (pSelf->GetPlayerChar(i)->m_IsBombing)
					{
						pSelf->SendChatTarget(i, aBuf);
					}
				}
			}
		}
	}
	else if (!str_comp_nocase(aCmd, "status"))
	{
		if (!pSelf->m_BombGameState) //offline
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Currently no bomb game running.");
			return;
		}
		else if (pSelf->m_BombGameState == 1) //lobby
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "------ Bomb Lobby ------");
			str_format(aBuf, sizeof(aBuf), "[%d/%d] players ready", pSelf->CountReadyBombPlayers(), pSelf->CountBombPlayers());
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendChatTarget(pResult->m_ClientID, "------------------------");
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (pSelf->GetPlayerChar(i) && pSelf->GetPlayerChar(i)->m_IsBombing)
				{
					if (pSelf->GetPlayerChar(i)->m_IsBombReady)
					{
						str_format(aBuf, sizeof(aBuf), "'%s' (ready)", pSelf->Server()->ClientName(i));
						pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
					}
					else
					{
						str_format(aBuf, sizeof(aBuf), "'%s'", pSelf->Server()->ClientName(i));
						pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
					}
				}
			}
			return;
		}
		else if (pSelf->m_BombGameState == 2) //lobby locked
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "------ Bomb Lobby (locked) ------");
			str_format(aBuf, sizeof(aBuf), "[%d/%d] players ready", pSelf->CountReadyBombPlayers(), pSelf->CountBombPlayers());
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendChatTarget(pResult->m_ClientID, "------------------------");
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (pSelf->GetPlayerChar(i) && pSelf->GetPlayerChar(i)->m_IsBombing)
				{
					if (pSelf->GetPlayerChar(i)->m_IsBombReady)
					{
						str_format(aBuf, sizeof(aBuf), "'%s' (ready)", pSelf->Server()->ClientName(i));
						pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
					}
					else
					{
						str_format(aBuf, sizeof(aBuf), "'%s'", pSelf->Server()->ClientName(i));
						pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
					}
				}
			}
			return;
		}
		else if (pSelf->m_BombGameState == 3) //ingame
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "------ Bomb (running game) ------");
			str_format(aBuf, sizeof(aBuf), "[%d/%d] players ready", pSelf->CountReadyBombPlayers(), pSelf->CountBombPlayers());
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendChatTarget(pResult->m_ClientID, "------------------------");
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (pSelf->GetPlayerChar(i) && pSelf->GetPlayerChar(i)->m_IsBombing)
				{
					if (pSelf->GetPlayerChar(i)->m_IsBombReady)
					{
						str_format(aBuf, sizeof(aBuf), "'%s' (ready)", pSelf->Server()->ClientName(i));
						pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
					}
					else
					{
						str_format(aBuf, sizeof(aBuf), "'%s'", pSelf->Server()->ClientName(i));
						pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
					}
				}
			}
			return;
		}
	}
	else if (!str_comp_nocase(aCmd, "ban"))
	{
		if (pResult->NumArguments() < 3)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "ERROR stick to following structure:");
			pSelf->SendChatTarget(pResult->m_ClientID, "'/bomb ban <seconds> <player>'");
			return;
		}

		if (pPlayer->m_Authed == CServer::AUTHED_ADMIN) //DESC power to use highest rank
		{
			//int Bantime = pResult->GetInteger(1) * pSelf->Server()->TickSpeed();
			int Bantime = pResult->GetInteger(1);
			char aBanname[32];
			str_copy(aBanname, pResult->GetString(2), sizeof(aBanname));
			int BanID = pSelf->GetCIDByName(aBanname);

			if (BanID == -1)
			{
				str_format(aBuf, sizeof(aBuf), "Can't find a user withe the name: '%s'", aBanname);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
				return;
			}

			if (Bantime > 900) //15 min ban limit
			{
				Bantime = 900;
				str_format(aBuf, sizeof(aBuf), "You banned '%s' for 15 minutes (max).", aBanname);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);

				//off all bomb stuff for the player
				pSelf->GetPlayerChar(BanID)->m_IsBombing = false;
				pSelf->GetPlayerChar(BanID)->m_IsBomb = false;
				pSelf->GetPlayerChar(BanID)->m_IsBombReady = false;

				//do the ban
				pSelf->m_apPlayers[BanID]->m_BombBanTime = Bantime * 60;
				str_format(aBuf, sizeof(aBuf), "[BOMB] You were banned by admin '%s' for %d seconds.", pSelf->Server()->ClientName(pResult->m_ClientID), Bantime);
				pSelf->SendChatTarget(BanID, aBuf);
			}
			else if (Bantime < 0)
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Bantime has to be positive.");
				return;
			}
			else
			{
				//BANNED PLAYER

				//off all bomb stuff for the player
				pSelf->GetPlayerChar(BanID)->m_IsBombing = false;
				pSelf->GetPlayerChar(BanID)->m_IsBomb = false;
				pSelf->GetPlayerChar(BanID)->m_IsBombReady = false;

				//do the ban
				pSelf->m_apPlayers[BanID]->m_BombBanTime = Bantime * 60;
				str_format(aBuf, sizeof(aBuf), "[BOMB] You were banned by admin '%s' for %d seconds.", pSelf->Server()->ClientName(pResult->m_ClientID), Bantime);
				pSelf->SendChatTarget(BanID, aBuf);

				//BANNING PLAYER
				str_format(aBuf, sizeof(aBuf), "You banned '%s' for %d seconds from bomb games.", aBanname, Bantime);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);

				//ADMIN CONSOLE (isnt admin console ._. itz chat :c)
				//str_format(aBuf, sizeof(aBuf), "'%s' were banned by admin '%s' for %d seconds.", aBanname, pSelf->Server()->ClientName(pResult->m_ClientID), Bantime);
				//pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "bomb", aBuf);
			}
		}
		else if (pPlayer->m_IsSuperModerator)
		{
			int Bantime = pResult->GetInteger(1);
			char aBanname[32];
			str_copy(aBanname, pResult->GetString(2), sizeof(aBanname));
			int BanID = pSelf->GetCIDByName(aBanname);

			if (BanID == -1)
			{
				str_format(aBuf, sizeof(aBuf), "Can't find a user withe the name: '%s'", aBanname);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
				return;
			}
			if (pSelf->m_apPlayers[BanID]->m_Authed == CServer::AUTHED_ADMIN)
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission to kick this player.");
				return;
			}

			if (Bantime > 300) //5 min ban limit
			{
				Bantime = 300;
				str_format(aBuf, sizeof(aBuf), "You banned '%s' for 5 minutes (max).", aBanname);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);

				//off all bomb stuff for the player
				pSelf->GetPlayerChar(BanID)->m_IsBombing = false;
				pSelf->GetPlayerChar(BanID)->m_IsBomb = false;
				pSelf->GetPlayerChar(BanID)->m_IsBombReady = false;

				//do the ban
				pSelf->m_apPlayers[BanID]->m_BombBanTime = Bantime * 60;
				str_format(aBuf, sizeof(aBuf), "[BOMB] You were banned by supermoderator '%s' for %d seconds.", pSelf->Server()->ClientName(pResult->m_ClientID), Bantime);
				pSelf->SendChatTarget(BanID, aBuf);
			}
			else if (Bantime < 0)
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Bantime has to be positive.");
				return;
			}
			else
			{
				//BANNED PLAYER

				//off all bomb stuff for the player
				pSelf->GetPlayerChar(BanID)->m_IsBombing = false;
				pSelf->GetPlayerChar(BanID)->m_IsBomb = false;
				pSelf->GetPlayerChar(BanID)->m_IsBombReady = false;

				//do the ban
				pSelf->m_apPlayers[BanID]->m_BombBanTime = Bantime * 60;
				str_format(aBuf, sizeof(aBuf), "[BOMB] You were banned by supermoderator '%s' for %d seconds.", pSelf->Server()->ClientName(pResult->m_ClientID), Bantime);
				pSelf->SendChatTarget(BanID, aBuf);

				//BANNING PLAYER
				str_format(aBuf, sizeof(aBuf), "You banned '%s' for %d seconds from bomb games.", aBanname, Bantime);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);

				//ADMIN CONSOLE (isnt admin console ._. itz chat :c)
				//str_format(aBuf, sizeof(aBuf), "'%s' were banned by supermoderator '%s' for %d seconds.", aBanname, pSelf->Server()->ClientName(pResult->m_ClientID), Bantime);
				//pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "bomb", aBuf);
			}
		}
		else if (pPlayer->m_IsModerator)
		{
			int Bantime = pResult->GetInteger(1);
			char aBanname[32];
			str_copy(aBanname, pResult->GetString(2), sizeof(aBanname));
			int BanID = pSelf->GetCIDByName(aBanname);

			if (BanID == -1)
			{
				str_format(aBuf, sizeof(aBuf), "Can't find a user withe the name: '%s'", aBanname);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
				return;
			}
			if (pSelf->m_apPlayers[BanID]->m_Authed == CServer::AUTHED_ADMIN || pSelf->m_apPlayers[BanID]->m_IsSuperModerator)
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission to kick this player.");
				return;
			}

			if (Bantime > 60) //1 min ban limit
			{
				Bantime = 60;
				str_format(aBuf, sizeof(aBuf), "You banned '%s' for 1 minutes (max).", aBanname);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);

				//off all bomb stuff for the player
				pSelf->GetPlayerChar(BanID)->m_IsBombing = false;
				pSelf->GetPlayerChar(BanID)->m_IsBomb = false;
				pSelf->GetPlayerChar(BanID)->m_IsBombReady = false;

				//do the ban
				pSelf->m_apPlayers[BanID]->m_BombBanTime = Bantime * 60;
				str_format(aBuf, sizeof(aBuf), "[BOMB] You were banned by moderator '%s' for %d seconds.", pSelf->Server()->ClientName(pResult->m_ClientID), Bantime);
				pSelf->SendChatTarget(BanID, aBuf);
			}
			else if (Bantime < 0)
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Bantime has to be positive.");
				return;
			}
			else
			{
				//BANNED PLAYER

				//off all bomb stuff for the player
				pSelf->GetPlayerChar(BanID)->m_IsBombing = false;
				pSelf->GetPlayerChar(BanID)->m_IsBomb = false;
				pSelf->GetPlayerChar(BanID)->m_IsBombReady = false;

				//do the ban
				pSelf->m_apPlayers[BanID]->m_BombBanTime = Bantime * 60;
				str_format(aBuf, sizeof(aBuf), "[BOMB] You were banned by moderator '%s' for %d seconds.", pSelf->Server()->ClientName(pResult->m_ClientID), Bantime);
				pSelf->SendChatTarget(BanID, aBuf);

				//BANNING PLAYER
				str_format(aBuf, sizeof(aBuf), "You banned '%s' for %d seconds from bomb games.", aBanname, Bantime);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);

				//ADMIN CONSOLE (isnt admin console ._. itz chat :c)
				//str_format(aBuf, sizeof(aBuf), "'%s' were banned by moderator '%s' for %d seconds.", aBanname, pSelf->Server()->ClientName(pResult->m_ClientID), Bantime);
				//pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "bomb", aBuf);
			}
		}
		else if (pPlayer->m_Authed == CServer::AUTHED_MOD)
		{
			int Bantime = pResult->GetInteger(1);
			char aBanname[32];
			str_copy(aBanname, pResult->GetString(2), sizeof(aBanname));
			int BanID = pSelf->GetCIDByName(aBanname);

			if (BanID == -1)
			{
				str_format(aBuf, sizeof(aBuf), "Can't find a user withe the name: '%s'", aBanname);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
				return;
			}
			if (pSelf->m_apPlayers[BanID]->m_Authed == CServer::AUTHED_ADMIN || pSelf->m_apPlayers[BanID]->m_IsSuperModerator || pSelf->m_apPlayers[BanID]->m_IsModerator)
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission to kick this player.");
				return;
			}

	
			if (!pSelf->GetPlayerChar(BanID)->m_IsBombing)
			{
				str_format(aBuf, sizeof(aBuf), "'%s' is not in a bomb game.", aBanname);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
				return;
			}

			//KICKED PLAYER

			//off all bomb stuff for the player
			pSelf->GetPlayerChar(BanID)->m_IsBombing = false;
			pSelf->GetPlayerChar(BanID)->m_IsBomb = false;
			pSelf->GetPlayerChar(BanID)->m_IsBombReady = false;
			pSelf->SendBroadcast("", BanID);

			//do the kick
			pSelf->m_apPlayers[BanID]->m_BombBanTime = Bantime * 60;
			str_format(aBuf, sizeof(aBuf), "[BOMB] You were kicked by rcon_mod '%s'.", pSelf->Server()->ClientName(pResult->m_ClientID));
			pSelf->SendChatTarget(BanID, aBuf);

			//KICKING PLAYER
			str_format(aBuf, sizeof(aBuf), "You kicked '%s' from bomb games.", aBanname);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);

			//ADMIN CONSOLE (isnt admin console ._. itz chat :c)
			//str_format(aBuf, sizeof(aBuf), "'%s' were kicked by rcon_mod '%s'.", aBanname, pSelf->Server()->ClientName(pResult->m_ClientID));
			//pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "bomb", aBuf);

		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission.");
			return;
		}
	}
	else if (!str_comp_nocase(aCmd, "unban"))
	{
		if (pResult->NumArguments() < 2)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Missing parameter: ClientID to unban ('/bomb banlist' for IDs).");
			pSelf->SendChatTarget(pResult->m_ClientID, "Command structure: '/bomb unban <ClientID>'");
			pSelf->SendChatTarget(pResult->m_ClientID, "Unban all: '/bomb unban -1'");
			return;
		}

		int UnbanID = pResult->GetInteger(1);

		if (UnbanID == -1) //unban all
		{
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->m_BombBanTime)
				{
					//UNBANNING PLAYER
					str_format(aBuf, sizeof(aBuf), "You unbanned '%s' (he had %d seconds bantime left).", pSelf->Server()->ClientName(i), pSelf->m_apPlayers[i]->m_BombBanTime / 60);
					pSelf->SendChatTarget(pResult->m_ClientID, aBuf);

					//UNBANNED PLAYER
					pSelf->m_apPlayers[i]->m_BombBanTime = 0;
					str_format(aBuf, sizeof(aBuf), "You were unbanned from bomb games by '%s'.", pSelf->Server()->ClientName(pResult->m_ClientID));
					pSelf->SendChatTarget(pSelf->m_apPlayers[i]->GetCID(), aBuf);
				}
			}
			return;
		}


		if (pSelf->m_apPlayers[UnbanID])
		{
			if (pSelf->m_apPlayers[UnbanID]->m_BombBanTime)
			{
				//UNBANNING PLAYER
				str_format(aBuf, sizeof(aBuf), "You unbanned '%s' (he had %d seconds bantime left).", pSelf->Server()->ClientName(UnbanID), pSelf->m_apPlayers[UnbanID]->m_BombBanTime / 60);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);

				//UNBANNED PLAYER
				pSelf->m_apPlayers[UnbanID]->m_BombBanTime = 0;
				str_format(aBuf, sizeof(aBuf), "You were unbanned from bomb games by '%s'.", pSelf->Server()->ClientName(pResult->m_ClientID));
				pSelf->SendChatTarget(pSelf->m_apPlayers[UnbanID]->GetCID(), aBuf);
			}
			else
			{
				str_format(aBuf, sizeof(aBuf), "'%s' is not banned from bomb games.", pSelf->Server()->ClientName(UnbanID));
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			}
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "There is no player with this ClientID.");
			return;
		}
	}
	else if (!str_comp_nocase(aCmd, "banlist")) //for now all dudes can see the banlist... not sure to make it rank only visible
	{
		if (!pSelf->CountBannedBombPlayers())
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "There are no banned bomb players yay.");
			return;
		}


		//calcualte page amount
		//float f_pages = pSelf->CountBannedBombPlayers() / 5;

		//int pages = round(pSelf->CountBannedBombPlayers() / 5 + 0.5); //works as good as ++
		int pages = (int)((float)pSelf->CountBannedBombPlayers() / 5.0f + 0.999999f);
		int PlayersShownOnPage = 0;


		if (pResult->NumArguments() > 1) //show pages
		{
			int PlayersShownOnPreviousPages = 0;
			int page = pResult->GetInteger(1);
			if (page > pages)
			{
				if (pages == 1)
				{
					str_format(aBuf, sizeof(aBuf), "Error there is only %d page.", pages);
					pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
				}
				else
				{
					str_format(aBuf, sizeof(aBuf), "Error there are only %d pages.", pages);
					pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
				}
			}
			else
			{
				str_format(aBuf, sizeof(aBuf), "=== (%d) Banned Bombers (page %d/%d) ===", pSelf->CountBannedBombPlayers(), page, pages);
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
				//for (int i = page * 5; i < MAX_CLIENTS; i++) yes it is an minor performance improvement but fuck it (did that cuz something didnt work) ((would be -1 anyways because human page 2 is computer page 1))
				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					if (pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->m_BombBanTime)
					{
						if (PlayersShownOnPreviousPages >= (page - 1) * 5)
						{
							str_format(aBuf, sizeof(aBuf), "ID: %d '%s' (%d seconds)", pSelf->m_apPlayers[i]->GetCID(), pSelf->Server()->ClientName(i), pSelf->m_apPlayers[i]->m_BombBanTime / 60);
							pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
							PlayersShownOnPage++;
						}
						else
						{
							//str_format(aBuf, sizeof(aBuf), "No list cuz  NOT Previous: %d > (page - 1) * 5: %d", PlayersShownOnPreviousPages, (page - 1) * 5);
							//dbg_msg("bomb", aBuf);
							PlayersShownOnPreviousPages++;
						}
					}
					if (PlayersShownOnPage > 4) //show only 5 players on one site
					{
						//dbg_msg("bomb", "page limit reached");
						break;
					}
				}
				//str_format(aBuf, sizeof(aBuf), "Finished loop. Players on previous: %d", PlayersShownOnPreviousPages);
				//dbg_msg("bomb", aBuf);
			}
		}
		else //show page one
		{
			str_format(aBuf, sizeof(aBuf), "=== (%d) Banned Bombers (page %d/%d) ===", pSelf->CountBannedBombPlayers(), 1, pages);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->m_BombBanTime)
				{
					str_format(aBuf, sizeof(aBuf), "ID: %d '%s' (%d seconds)", pSelf->m_apPlayers[i]->GetCID(), pSelf->Server()->ClientName(i), pSelf->m_apPlayers[i]->m_BombBanTime / 60);
					pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
					PlayersShownOnPage++;
				}
				if (PlayersShownOnPage > 4) //show only 5 players on one site
				{
					break;
				}
			}
		}
	}
	else if (!str_comp_nocase(aCmd, "help") || !str_comp_nocase(aCmd, "info"))
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "===================");
		pSelf->SendChatTarget(pResult->m_ClientID, "->   B O M B   <-");
		pSelf->SendChatTarget(pResult->m_ClientID, "===================");
		pSelf->SendChatTarget(pResult->m_ClientID, "HOW DOES THE GAME WORK?");
		pSelf->SendChatTarget(pResult->m_ClientID, "One player get's choosen as bomb. He can give the bomb to other players with his hammer.");
		pSelf->SendChatTarget(pResult->m_ClientID, "If a player explodes as bomb he is out.");
		pSelf->SendChatTarget(pResult->m_ClientID, "Last man standing is the winner.");
		pSelf->SendChatTarget(pResult->m_ClientID, "HOW TO START?");
		pSelf->SendChatTarget(pResult->m_ClientID, "One player has to create a bomb lobby with '/bomb create <money>'.");
		pSelf->SendChatTarget(pResult->m_ClientID, "Then other players can join with '/bomb join'.");
		pSelf->SendChatTarget(pResult->m_ClientID, "All players who join the game have to pay <money> money and the winner gets it all.");
		pSelf->SendChatTarget(pResult->m_ClientID, "---------------");
		pSelf->SendChatTarget(pResult->m_ClientID, "more bomb commands at '/bomb cmdlist'");
		return;
	}
	else if (!str_comp_nocase(aCmd, "cmdlist"))
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "=== BOMB COMMANDS ===");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/bomb create <money>' to create a bomb game.");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/bomb create <money> <map>' also creates a bomb game.");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/bomb join' to join a bomb game.");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/bomb start' to start a game.");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/bomb lock' to lock a bomb lobby.");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/bomb status' to see some live bomb stats.");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/bomb ban <seconds> <name>' to ban players.");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/bomb unban <ClientID>' to unban players.");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/bomb help' for help and info.");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/bomb cmdlist' for all bomb commands.");
		return;
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Unknown bomb command. More help at '/bomb help' or 'bomb cmdlist'");
		return;
	}
}

void CGameContext::ConRoom(IConsole::IResult * pResult, void * pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	char aBuf[128];
	char aCmd[64];
	str_copy(aCmd, pResult->GetString(0), sizeof(aCmd));

	if (!str_comp_nocase(aCmd, "help"))
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Help helps...");
	}
	else if (!str_comp_nocase(aCmd, "invite"))
	{
		if (pResult->NumArguments() < 2)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "ERROR stick struct: '/room invite <player>'.");
			return;
		}

		char aInviteName[32];
		str_copy(aInviteName, pResult->GetString(1), sizeof(aInviteName));
		int InviteID = pSelf->GetCIDByName(aInviteName);

		if (InviteID == -1)
		{
			str_format(aBuf, sizeof(aBuf), "Can't find the player '%s'.", aInviteName);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			return;
		}
		if (!pPlayer->m_IsSuperModerator)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission.");
			return;
		}
		if (!pPlayer->m_BoughtRoom)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You need a roomkey to invite others. ('/buy room_key')");
			return;
		}
		if (pSelf->m_apPlayers[InviteID]->m_BoughtRoom)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "This player already bought a room key.");
			return;
		}
		if (pSelf->GetPlayerChar(InviteID)->m_HasRoomKeyBySuperModerator)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "This player already were given a key by a SuperModerator.");
			return;
		}


		//GETTER
		pSelf->GetPlayerChar(InviteID)->m_HasRoomKeyBySuperModerator = true;
		str_format(aBuf, sizeof(aBuf), "'%s' invited you to the room c:", pSelf->Server()->ClientName(pResult->m_ClientID));
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		//GIVER
		str_format(aBuf, sizeof(aBuf), "You invited '%s' to the room", pSelf->Server()->ClientName(InviteID));
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Unknow command try '/room help' for more help.");
	}
}

void CGameContext::ConBank(IConsole::IResult * pResult, void * pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	char aBuf[256];

	if (pResult->NumArguments() == 0)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "**** BANK ****");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/bank close'");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/bank open'");
		pSelf->SendChatTarget(pResult->m_ClientID, "banks are sensless af...");
		return;
	}

	if (!str_comp_nocase(pResult->GetString(0), "close"))
	{
		if (pPlayer->m_Authed != CServer::AUTHED_ADMIN) 
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "missing permission to use this command.");
			return;
		}

		if (!pSelf->m_IsBankOpen)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Bank is already closed.");
			return;
		}

		pSelf->m_IsBankOpen = false;
		pSelf->SendChatTarget(pResult->m_ClientID, "<bank> bye world!");
	}
	else if (!str_comp_nocase(pResult->GetString(0), "open"))
	{
		if (pPlayer->m_Authed != CServer::AUTHED_ADMIN)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "missing permission to use this command.");
			return;
		}

		if (pSelf->m_IsBankOpen)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Bank is already open.");
			return;
		}

		pSelf->m_IsBankOpen = true;
		pSelf->SendChatTarget(pResult->m_ClientID, "<bank> hello world!");
	}
	else if (!str_comp_nocase(pResult->GetString(0), "rob"))
	{
		if (!pChr->m_InBank)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You have to be in a bank.");
			return;
		}
		if (!pSelf->m_IsBankOpen)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Bank is closed.");
			return;
		}
		if (pPlayer->m_AccountID <= 0)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You are not logged in more info at '/accountinfo'.");
			return;
		}

		int policedudesfound = 0;
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->m_PoliceRank && pSelf->m_apPlayers[i] != pPlayer)
			{
				policedudesfound++;
			}
		}

		if (!policedudesfound)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You robbed the bank. (bank was empty)");
			return;
		}

		pPlayer->m_EscapeTime += pSelf->Server()->TickSpeed() * 600; //+10 min
		//str_format(aBuf, sizeof(aBuf), "+%d bank robbery", 5 * policedudesfound);
		//pPlayer->MoneyTransaction(+5 * policedudesfound, aBuf);
		pPlayer->m_GangsterBagMoney += 5 * policedudesfound;
		str_format(aBuf, sizeof(aBuf), "You robbed the bank. (+%d money to your gangstabag)", 5 * policedudesfound);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		str_format(aBuf, sizeof(aBuf), "Police will be hunting you for %d minutes.", (pPlayer->m_EscapeTime / pSelf->Server()->TickSpeed()) / 60);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);

		str_format(aBuf, sizeof(aBuf), "'%s' robbed the bank.", pSelf->Server()->ClientName(pResult->m_ClientID));
		pSelf->SendAllPolice(aBuf);
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "unknown bank command check '/bank' for more help.");
	}
}

void CGameContext::ConGangsterBag(IConsole::IResult * pResult, void * pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "real gangstas aren't dead or spectator.");
		return;
	}

	char aBuf[256];

	if (pResult->NumArguments() == 0)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, ")&(%)59 GAng$st4 Bag (?=/(§");
		str_format(aBuf, sizeof(aBuf), "%d gAng$sta coins in your bag m8 ", pPlayer->m_GangsterBagMoney);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		pSelf->SendChatTarget(pResult->m_ClientID, "gangsta coins are rip on disconnect");
		pSelf->SendChatTarget(pResult->m_ClientID, "real gangster play 24/7 or do illegal 7gangsterbag trade(s)-");
		return;
	}

	if (!str_comp_nocase(pResult->GetString(0), "trade"))
	{
		//todo: add trades with hammer to give gangsta coins to others

		// cant send yourself

		// can only trade if no escapetime

		// use brain to find bugsis

		if (!pPlayer->m_GangsterBagMoney)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "you no coins mate");
			return;
		}
		if (pResult->NumArguments() < 2)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "dude use: trade <gangsta broo>");
			return;
		}
		if (pPlayer->m_EscapeTime)
		{
			str_format(aBuf, sizeof(aBuf), "You can't trade while escaping the police. You have to wait %d seconds...", pPlayer->m_EscapeTime / pSelf->Server()->TickSpeed());
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			return;
		}

		int broID = pSelf->GetCIDByName(pResult->GetString(1));
		if (broID == -1)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "gettin crazy m8? Choose a real person...");
			return;
		}
		if (pSelf->m_apPlayers[broID]->m_AccountID <= 0)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "sure this is a trusty trade bro? He is not logged in -.-");
			return;
		}
		if (broID == pResult->m_ClientID)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "real gangster only traderino w other ppls");
			return;
		}
		char aOwnIP[128];
		char aBroIP[128];
		pSelf->Server()->GetClientAddr(pResult->m_ClientID, aOwnIP, sizeof(aOwnIP));
		pSelf->Server()->GetClientAddr(broID, aBroIP, sizeof(aBroIP));

		if (!str_comp_nocase(aOwnIP, aBroIP)) //send dummy money -> police traces ip -> dummy escape time 
		{
			//bro
			pSelf->m_apPlayers[broID]->m_GangsterBagMoney += pPlayer->m_GangsterBagMoney;
			pSelf->m_apPlayers[broID]->m_EscapeTime += pSelf->Server()->TickSpeed() * 180; // 180 secs == 3 minutes
			str_format(aBuf, sizeof(aBuf), "'%s' traded you %d gangster coins (police traced ip)", pSelf->Server()->ClientName(pResult->m_ClientID), pPlayer->m_GangsterBagMoney);
			pSelf->SendChatTarget(broID, aBuf);

			//trader
			pSelf->SendChatTarget(pResult->m_ClientID, "Police recognized the illegal trade -.- (ip traced)");
			pSelf->SendChatTarget(pResult->m_ClientID, "Your bro has now gangster coins and is getting hunted by police.");
			pPlayer->m_GangsterBagMoney = 0;
			pPlayer->m_EscapeTime += pSelf->Server()->TickSpeed() * 60; // +1 minutes for illegal trades
			return;
		}
		else
		{
			str_format(aBuf, sizeof(aBuf), "'%s' traded you %d money (totally legal ^^)", pSelf->Server()->ClientName(pResult->m_ClientID), pPlayer->m_GangsterBagMoney);
			pSelf->SendChatTarget(broID, aBuf);
			str_format(aBuf, sizeof(aBuf), "+%d (unknown source)", pPlayer->m_GangsterBagMoney);
			pSelf->m_apPlayers[broID]->MoneyTransaction(+pPlayer->m_GangsterBagMoney, aBuf);

			pPlayer->m_GangsterBagMoney = 0;
			pSelf->SendChatTarget(pResult->m_ClientID, "traded gangsta coins !!!");
		}
	}
	else if (!str_comp_nocase(pResult->GetString(0), "clear"))
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "cleared gangsterbag ... rip coins ._.");
		pPlayer->m_GangsterBagMoney = 0;
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "real gangstas no makin typos");
	}
}
void CGameContext::ConJail(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	char aBuf[256];

	if (pResult->NumArguments() == 0)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "---- JAIL ----");
		pSelf->SendChatTarget(pResult->m_ClientID, "The police brings all the gangster here.");
		//pSelf->SendChatTarget(pResult->m_ClientID, "");
		return;
	}

	if (!str_comp_nocase(pResult->GetString(0), "open"))
	{
		if (pResult->NumArguments() < 3)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Missing parameters. '/jail open <code> <player>'");
			return;
		}
		if (!pSelf->IsPosition(pResult->m_ClientID, 0))
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Get closer to the cell.");
			return;
		}
		char aBuf[256];
		int jailedID = pSelf->GetCIDByName(pResult->GetString(2));
		if (!pSelf->m_apPlayers[jailedID])
		{
			str_format(aBuf, sizeof(aBuf), "'%s' is not online.", pResult->GetString(2));
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			return;
		}
		if (!pSelf->m_apPlayers[jailedID]->m_JailTime)
		{
			str_format(aBuf, sizeof(aBuf), "'%s' is not arrested.", pResult->GetString(2));
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			return;
		}
		if (pResult->GetInteger(1) != pSelf->m_apPlayers[jailedID]->m_JailCode)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "wrong cell code.");
			return;
		}

		str_format(aBuf, sizeof(aBuf), "you opend '%s' cell.", pResult->GetString(2));
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		
		pSelf->m_apPlayers[jailedID]->m_IsJailDoorOpen = true;
		str_format(aBuf, sizeof(aBuf), "your cell door was opend by '%s'.", pSelf->Server()->ClientName(pResult->m_ClientID));
		pSelf->SendChatTarget(jailedID, aBuf);
		pSelf->SendChatTarget(jailedID, "'/jail leave' to leave. (warning this might be illegal)");
	}
	//else if (!str_comp_nocase(pResult->GetString(0), "corrupt"))
	//{
	//	if (pResult->NumArguments() == 1)
	//	{
	//		pSelf->SendChatTarget(pResult->m_ClientID, "'/jail corrupt <money> <player>'");
	//		return;
	//	}

	//	int corruptID = pSelf->GetCIDByName(pResult->GetString(2));
	//	if (corruptID == -1)
	//	{
	//		str_format(aBuf, sizeof(aBuf), "'%s' is not online.", pResult->GetString(2));
	//		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	//		return;
	//	}
	//	if (!pSelf->m_apPlayers[corruptID]->m_PoliceRank)
	//	{
	//		str_format(aBuf, sizeof(aBuf), "'%s' is no police officer.", pResult->GetString(2));
	//		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	//		return;
	//	}

	//	str_format(aBuf, sizeof(aBuf), "you offered %s %d money to reduce your jailtime.", pResult->GetString(2), pResult->GetInteger(1));
	//	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);

	//	str_format(aBuf, sizeof(aBuf), "'%s' would pay %d money if you help with an escape.", pResult->GetString(2), pResult->GetInteger(1));
	//	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	//	pSelf->SendChatTarget(pResult->m_ClientID, "'/jail release <jail code> %s' to take the money.");
	//}
	else if (!str_comp_nocase(pResult->GetString(0), "list")) //codes
	{
		if (pPlayer->m_JailTime || !pPlayer->m_PoliceRank)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "missing permission.");
			return;
		}

		//list all jailed players with codes on several pages (steal bomb system)
	}
	else if (!str_comp_nocase(pResult->GetString(0), "code"))
	{
		if (pPlayer->m_PoliceRank < 2)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "you need police rank 2 or higher.");
			return;
		}
		if (pPlayer->m_JailTime)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "you are arrested.");
			return;
		}
		if (pResult->NumArguments() < 2)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "use '/jail code <client-id>'");
			return;
		}
		if (!pSelf->m_apPlayers[pResult->GetInteger(1)])
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "No player with this ID online.");
			return;
		}
		if (!pSelf->m_apPlayers[pResult->GetInteger(1)]->m_JailTime)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "this player is not arrested.");
			return;
		}

		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "'%s' [%d]", pSelf->Server()->ClientName(pResult->GetInteger(1)), pSelf->m_apPlayers[pResult->GetInteger(1)]->m_JailCode);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
	else if (!str_comp_nocase(pResult->GetString(0), "leave"))
	{
		if (!pPlayer->m_JailTime)
		{
			//pSelf->SendChatTarget(pResult->m_ClientID, "you are not arrested.");
			return;
		}
		if (!pPlayer->m_IsJailDoorOpen)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "cell door is closed.");
			return;
		}

		pSelf->SendChatTarget(pResult->m_ClientID, "You escaped the jail, run! The police will be searching you for 10 minutes.");
		pPlayer->m_EscapeTime = pSelf->Server()->TickSpeed() * 600; // 10 minutes for escaping the jail
		pPlayer->m_JailTime = 0;
		pPlayer->m_IsJailDoorOpen = false;
	}
	else if (!str_comp_nocase(pResult->GetString(0), "hammer"))
	{
		if (!pPlayer->m_PoliceRank)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You have to be police to use this command.");
			return;
		}
		if (pResult->NumArguments() == 1)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "++++ Jail Hammer ++++");
			pSelf->SendChatTarget(pResult->m_ClientID, "Use this command to configurate your hammer.");
			pSelf->SendChatTarget(pResult->m_ClientID, "Use this hammer to move gangsters to jail.");
			pSelf->SendChatTarget(pResult->m_ClientID, "Simply activate it and hammer escaping gangsters.");
			pSelf->SendChatTarget(pResult->m_ClientID, "(you can only use it on freezed gangsters)");
			pSelf->SendChatTarget(pResult->m_ClientID, "If you are police 5 or higher then you can activate jail all hammer.");
			pSelf->SendChatTarget(pResult->m_ClientID, "Then you can jail also tees who are not known as gangsters.");
			pSelf->SendChatTarget(pResult->m_ClientID, "-- commands --");
			pSelf->SendChatTarget(pResult->m_ClientID, "'/jail hammer 1' to activate it.");
			pSelf->SendChatTarget(pResult->m_ClientID, "'/jail hammer 0' to deactivate it.");
			pSelf->SendChatTarget(pResult->m_ClientID, "'/jail hammer <seconds>' to activate jail all hammer.");
			return;
		}
		if (pResult->GetInteger(1) < 0)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "value has to be positive.");
			return;
		}
		if (pResult->GetInteger(1) == 0)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "jail hammer is now deactivated.");
			pPlayer->m_JailHammer = false;
			return;
		}
		if (pResult->GetInteger(1) == 1)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "jail hammer is now activated. (hammer frozen gangsters)");
			pPlayer->m_JailHammer = true;
			return;
		}
		if (pResult->GetInteger(1) > 1)
		{
			if (pPlayer->m_PoliceRank < 5)
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "you have to be police 5 or higher to use this value.");
				return;
			}
			if (pResult->GetInteger(1) > 600)
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "you can't arrest longer than 10 minutes.");
				return;
			}

			//pPlayer->m_JailHammerTime = pResult->GetInteger(1);
			pPlayer->m_JailHammer = pResult->GetInteger(1);
			str_format(aBuf, sizeof(aBuf), "you can now jail every freezed tee for %d seconds with your hammer.", pPlayer->m_JailHammer);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendChatTarget(pResult->m_ClientID, "You have to judge who is criminal and who not.");
			pSelf->SendChatTarget(pResult->m_ClientID, "A lot of power brings even more responsibillity.");
		}
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "unknown jail parameter check '/jail' for more info");
	}
}
void CGameContext::ConAscii(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	char aBuf[256];


	if (pResult->NumArguments() == 0)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "---- ascii art animation ----");
		pSelf->SendChatTarget(pResult->m_ClientID, "Create your own animation with this command.");
		pSelf->SendChatTarget(pResult->m_ClientID, "And publish it on your profile to share it.");
		pSelf->SendChatTarget(pResult->m_ClientID, "---- commands ----");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/ascii frame <frame number> <ascii art>' to edit a frame from 0-15");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/ascii speed <speed>' to change the animation speed");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/ascii profile <0/1>' private/publish animation on profile");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/ascii public <0/1>' private/publish animation");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/ascii view <client id>' to watch published animation");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/ascii view' to watch your animation");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/ascii stop' to stop running animation you'r watching");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/ascii stats' to see personal stats");
		return;
	}

	if (!str_comp_nocase(pResult->GetString(0), "stats"))
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "---- ascii stats ----");
		str_format(aBuf, sizeof(aBuf), "views: %d", pPlayer->m_AsciiViewsDefault + pPlayer->m_AsciiViewsProfile);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		pSelf->SendChatTarget(pResult->m_ClientID, "---- specific ----");
		str_format(aBuf, sizeof(aBuf), "ascii views: %d", pPlayer->m_AsciiViewsDefault);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		str_format(aBuf, sizeof(aBuf), "ascii views (profile): %d", pPlayer->m_AsciiViewsProfile);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
	else if (!str_comp_nocase(pResult->GetString(0), "stop"))
	{
		if (pPlayer->m_AsciiWatchingID == -1)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You are not watching an ascii animation.");
			return;
		}

		pPlayer->m_AsciiWatchingID = -1;
		pPlayer->m_AsciiWatchFrame = 0;
		pPlayer->m_AsciiWatchTicker = 0;
		pSelf->SendBroadcast("", pResult->m_ClientID);
	}
	else if (!str_comp_nocase(pResult->GetString(0), "profile"))
	{
		if (pPlayer->m_AccountID <= 0)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You have to be logged in to create ascii animations.");
			pSelf->SendChatTarget(pResult->m_ClientID, "Use '/accountinfo' for more help about accounts.");
			return;
		}

		if (pResult->NumArguments() != 2)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "'/ascii profile <0/1>' private/publish animation on profile");
			return;
		}

		if (pResult->GetInteger(1) == 0)
		{
			if (pPlayer->m_aAsciiPublishState[1] == '0')
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Your animation is already private.");
			}
			else
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Your animation is now private.");
				pSelf->SendChatTarget(pResult->m_ClientID, "It can be no longer watched with '/profile view <you>'");
				pPlayer->m_aAsciiPublishState[1] = '0';
			}
		}
		else if (pResult->GetInteger(1) == 1)
		{
			if (pPlayer->m_aAsciiPublishState[1] == '1')
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Your animation is already public on your profile.");
			}
			else
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Your animation is now public.");
				pSelf->SendChatTarget(pResult->m_ClientID, "It can be watched with '/profile view <you>'");
				pPlayer->m_aAsciiPublishState[1] = '1';
			}
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Use 0 to make your animation private.");
			pSelf->SendChatTarget(pResult->m_ClientID, "Use 1 to make your animation public.");
		}
	}
	else if (!str_comp_nocase(pResult->GetString(0), "public") || !str_comp_nocase(pResult->GetString(0), "publish"))
	{
		if (pPlayer->m_AccountID <= 0)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You have to be logged in to create ascii animations.");
			pSelf->SendChatTarget(pResult->m_ClientID, "Use '/accountinfo' for more help about accounts.");
			return;
		}

		if (pResult->NumArguments() != 2)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "'/ascii public <0/1>' private/publish animation");
			return;
		}

		if (pResult->GetInteger(1) == 0)
		{
			if (pPlayer->m_aAsciiPublishState[0] == '0')
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Your animation is already private.");
			}
			else
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Your animation is now private.");
				pSelf->SendChatTarget(pResult->m_ClientID, "It can be no longer watched with '/ascii view <your id>'");
				pPlayer->m_aAsciiPublishState[0] = '0';
			}
		}
		else if (pResult->GetInteger(1) == 1)
		{
			if (pPlayer->m_aAsciiPublishState[0] == '1')
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Your animation is already public.");
			}
			else
			{
				pSelf->SendChatTarget(pResult->m_ClientID, "Your animation is now public.");
				pSelf->SendChatTarget(pResult->m_ClientID, "It can be watched with '/ascii view <your id>'");
				pPlayer->m_aAsciiPublishState[0] = '1';
			}
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Use 0 to make your animation private.");
			pSelf->SendChatTarget(pResult->m_ClientID, "Use 1 to make your animation public.");
		}
	}
	else if (!str_comp_nocase(pResult->GetString(0), "frame"))
	{
		if (pPlayer->m_AccountID <= 0)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You have to be logged in to create ascii animations.");
			pSelf->SendChatTarget(pResult->m_ClientID, "Use '/accountinfo' for more help about accounts.");
			return;
		}

		if (pResult->NumArguments() < 2)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "'/ascii frame <frame number> <ascii art>' to edit a frame from 0-15");
			return;
		}

		if (pResult->GetInteger(1) == 0)
		{
			str_format(pPlayer->m_aAsciiFrame0, sizeof(pPlayer->m_aAsciiFrame0), "%s", pResult->GetString(2));
			str_format(aBuf, sizeof(aBuf), "updated frame[%d]: %s", pResult->GetInteger(1), pPlayer->m_aAsciiFrame0);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendBroadcast(pPlayer->m_aAsciiFrame0, pResult->m_ClientID);
		}
		else if (pResult->GetInteger(1) == 1)
		{
			str_format(pPlayer->m_aAsciiFrame1, sizeof(pPlayer->m_aAsciiFrame1), "%s", pResult->GetString(2));
			str_format(aBuf, sizeof(aBuf), "updated frame[%d]: %s", pResult->GetInteger(1), pPlayer->m_aAsciiFrame1);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendBroadcast(pPlayer->m_aAsciiFrame1, pResult->m_ClientID);
		}
		else if (pResult->GetInteger(1) == 2)
		{
			str_format(pPlayer->m_aAsciiFrame2, sizeof(pPlayer->m_aAsciiFrame2), "%s", pResult->GetString(2));
			str_format(aBuf, sizeof(aBuf), "updated frame[%d]: %s", pResult->GetInteger(1), pPlayer->m_aAsciiFrame2);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendBroadcast(pPlayer->m_aAsciiFrame2, pResult->m_ClientID);
		}
		else if (pResult->GetInteger(1) == 3)
		{
			str_format(pPlayer->m_aAsciiFrame3, sizeof(pPlayer->m_aAsciiFrame3), "%s", pResult->GetString(2));
			str_format(aBuf, sizeof(aBuf), "updated frame[%d]: %s", pResult->GetInteger(1), pPlayer->m_aAsciiFrame3);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendBroadcast(pPlayer->m_aAsciiFrame3, pResult->m_ClientID);
		}
		else if (pResult->GetInteger(1) == 4)
		{
			str_format(pPlayer->m_aAsciiFrame4, sizeof(pPlayer->m_aAsciiFrame4), "%s", pResult->GetString(2));
			str_format(aBuf, sizeof(aBuf), "updated frame[%d]: %s", pResult->GetInteger(1), pPlayer->m_aAsciiFrame4);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendBroadcast(pPlayer->m_aAsciiFrame4, pResult->m_ClientID);
		}
		else if (pResult->GetInteger(1) == 5)
		{
			str_format(pPlayer->m_aAsciiFrame5, sizeof(pPlayer->m_aAsciiFrame5), "%s", pResult->GetString(2));
			str_format(aBuf, sizeof(aBuf), "updated frame[%d]: %s", pResult->GetInteger(1), pPlayer->m_aAsciiFrame5);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendBroadcast(pPlayer->m_aAsciiFrame5, pResult->m_ClientID);
		}
		else if (pResult->GetInteger(1) == 6)
		{
			str_format(pPlayer->m_aAsciiFrame6, sizeof(pPlayer->m_aAsciiFrame6), "%s", pResult->GetString(2));
			str_format(aBuf, sizeof(aBuf), "updated frame[%d]: %s", pResult->GetInteger(1), pPlayer->m_aAsciiFrame6);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendBroadcast(pPlayer->m_aAsciiFrame6, pResult->m_ClientID);
		}
		else if (pResult->GetInteger(1) == 7)
		{
			str_format(pPlayer->m_aAsciiFrame7, sizeof(pPlayer->m_aAsciiFrame7), "%s", pResult->GetString(2));
			str_format(aBuf, sizeof(aBuf), "updated frame[%d]: %s", pResult->GetInteger(1), pPlayer->m_aAsciiFrame7);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendBroadcast(pPlayer->m_aAsciiFrame7, pResult->m_ClientID);
		}
		else if (pResult->GetInteger(1) == 8)
		{
			str_format(pPlayer->m_aAsciiFrame8, sizeof(pPlayer->m_aAsciiFrame8), "%s", pResult->GetString(2));
			str_format(aBuf, sizeof(aBuf), "updated frame[%d]: %s", pResult->GetInteger(1), pPlayer->m_aAsciiFrame8);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendBroadcast(pPlayer->m_aAsciiFrame8, pResult->m_ClientID);
		}
		else if (pResult->GetInteger(1) == 9)
		{
			str_format(pPlayer->m_aAsciiFrame9, sizeof(pPlayer->m_aAsciiFrame9), "%s", pResult->GetString(2));
			str_format(aBuf, sizeof(aBuf), "updated frame[%d]: %s", pResult->GetInteger(1), pPlayer->m_aAsciiFrame9);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendBroadcast(pPlayer->m_aAsciiFrame9, pResult->m_ClientID);
		}
		else if (pResult->GetInteger(1) == 10)
		{
			str_format(pPlayer->m_aAsciiFrame10, sizeof(pPlayer->m_aAsciiFrame10), "%s", pResult->GetString(2));
			str_format(aBuf, sizeof(aBuf), "updated frame[%d]: %s", pResult->GetInteger(1), pPlayer->m_aAsciiFrame10);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendBroadcast(pPlayer->m_aAsciiFrame10, pResult->m_ClientID);
		}
		else if (pResult->GetInteger(1) == 11)
		{
			str_format(pPlayer->m_aAsciiFrame11, sizeof(pPlayer->m_aAsciiFrame11), "%s", pResult->GetString(2));
			str_format(aBuf, sizeof(aBuf), "updated frame[%d]: %s", pResult->GetInteger(1), pPlayer->m_aAsciiFrame11);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendBroadcast(pPlayer->m_aAsciiFrame11, pResult->m_ClientID);
		}
		else if (pResult->GetInteger(1) == 12)
		{
			str_format(pPlayer->m_aAsciiFrame12, sizeof(pPlayer->m_aAsciiFrame12), "%s", pResult->GetString(2));
			str_format(aBuf, sizeof(aBuf), "updated frame[%d]: %s", pResult->GetInteger(1), pPlayer->m_aAsciiFrame12);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendBroadcast(pPlayer->m_aAsciiFrame12, pResult->m_ClientID);
		}
		else if (pResult->GetInteger(1) == 13)
		{
			str_format(pPlayer->m_aAsciiFrame13, sizeof(pPlayer->m_aAsciiFrame13), "%s", pResult->GetString(2));
			str_format(aBuf, sizeof(aBuf), "updated frame[%d]: %s", pResult->GetInteger(1), pPlayer->m_aAsciiFrame13);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendBroadcast(pPlayer->m_aAsciiFrame13, pResult->m_ClientID);
		}
		else if (pResult->GetInteger(1) == 14)
		{
			str_format(pPlayer->m_aAsciiFrame14, sizeof(pPlayer->m_aAsciiFrame14), "%s", pResult->GetString(2));
			str_format(aBuf, sizeof(aBuf), "updated frame[%d]: %s", pResult->GetInteger(1), pPlayer->m_aAsciiFrame14);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendBroadcast(pPlayer->m_aAsciiFrame14, pResult->m_ClientID);
		}
		else if (pResult->GetInteger(1) == 15)
		{
			str_format(pPlayer->m_aAsciiFrame15, sizeof(pPlayer->m_aAsciiFrame15), "%s", pResult->GetString(2));
			str_format(aBuf, sizeof(aBuf), "updated frame[%d]: %s", pResult->GetInteger(1), pPlayer->m_aAsciiFrame15);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			pSelf->SendBroadcast(pPlayer->m_aAsciiFrame15, pResult->m_ClientID);
		}
		else
		{
			str_format(aBuf, sizeof(aBuf), "%d is not a valid frame. choose between 0 and 15");
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		}
	}
	else if (!str_comp_nocase(pResult->GetString(0), "view") || !str_comp_nocase(pResult->GetString(0), "watch"))
	{
		if (pResult->NumArguments() == 1) //show own
		{
			pSelf->StartAsciiAnimation(pResult->m_ClientID, pResult->m_ClientID, -1);
			return;
		}

		pSelf->StartAsciiAnimation(pResult->m_ClientID, pResult->GetInteger(1), 0);
	}
	else if (!str_comp_nocase(pResult->GetString(0), "speed"))
	{
		if (pPlayer->m_AccountID <= 0)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You have to be logged in to create ascii animations.");
			pSelf->SendChatTarget(pResult->m_ClientID, "Use '/accountinfo' for more help about accounts.");
			return;
		}

		if (pResult->NumArguments() != 2)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "'/ascii speed <speed>' to change the animation speed");
			return;
		}
		if (pResult->GetInteger(1) < 1)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Animation speed has to be 1 or higher.");
			return;
		}

		pPlayer->m_AsciiAnimSpeed = pResult->GetInteger(1);
		pSelf->SendChatTarget(pResult->m_ClientID, "updated animation speed.");
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Unknown ascii command check '/ascii' for command list.");
	}
}



void CGameContext::ConHook(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	if (pResult->NumArguments() == 0)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "===== hook =====");
		pSelf->SendChatTarget(pResult->m_ClientID, "'/hook <power>'");
		pSelf->SendChatTarget(pResult->m_ClientID, "===== powers =====");
		pSelf->SendChatTarget(pResult->m_ClientID, "normal, rainbow, bloody");
		return;
	}


	if (!str_comp_nocase(pResult->GetString(0), "normal"))
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "You got normal hook.");
		pPlayer->m_HookPower = 0;
	}
	else if (!str_comp_nocase(pResult->GetString(0), "rainbow"))
	{
		if (pPlayer->m_IsSuperModerator || pPlayer->m_IsModerator)
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "You got rainbow hook.");
			pPlayer->m_HookPower = 1;
		}
		else
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "missing permission.");
		}
	}
	else if (!str_comp_nocase(pResult->GetString(0), "bloody"))
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "coming soon...");
		//pPlayer->m_HookPower = 2;
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "unknown power check '/hook' for a list of all powers.");
	}
}