/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
/* Based on Race mod stuff and tweaked by GreYFoX@GTi and others to fit our DDRace needs. */
/* CSqlScore class by Sushi */
#if defined(CONF_SQL)
#include <string.h>
#include <fstream>
#include <algorithm>

#include <engine/shared/config.h>
#include "../entities/character.h"
#include "../gamemodes/DDRace.h"
#include "sql_score.h"
#include <engine/shared/console.h>
#include "../save.h"

static LOCK gs_SqlLock = 0;

CSqlScore::CSqlScore(CGameContext *pGameServer) : m_pGameServer(pGameServer),
		m_pServer(pGameServer->Server()),
		m_pDatabase(g_Config.m_SvSqlDatabase),
		m_pPrefix(g_Config.m_SvSqlPrefix),
		m_pUser(g_Config.m_SvSqlUser),
		m_pPass(g_Config.m_SvSqlPw),
		m_pIp(g_Config.m_SvSqlIp),
		m_Port(g_Config.m_SvSqlPort)
{
	m_pDriver = NULL;
	str_copy(m_aMap, g_Config.m_SvMap, sizeof(m_aMap));
	ClearString(m_aMap);

	if(gs_SqlLock == 0)
		gs_SqlLock = lock_create();

	Init();
}

CSqlScore::~CSqlScore()
{
	lock_wait(gs_SqlLock);
	lock_unlock(gs_SqlLock);

	try
	{
		delete m_pStatement;
		delete m_pConnection;
		dbg_msg("sql", "sql connection disconnected");
	}
	catch (sql::SQLException &e)
	{
		dbg_msg("sql", "ERROR: no sql connection");
	}
}

bool CSqlScore::Connect()
{
	if (m_pDriver != NULL && m_pConnection != NULL)
	{
		try
		{
			// Connect to specific database
			m_pConnection->setSchema(m_pDatabase);
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("sql", "MySQL ERROR: %s", e.what());
			dbg_msg("sql", "ERROR: sql connection failed");
			return false;
		}
		return true;
	}

	try
	{
		char aBuf[256];

		m_pDriver = 0;
		m_pConnection = 0;
		m_pStatement = 0;

		sql::ConnectOptionsMap connection_properties;
		connection_properties["hostName"]      = sql::SQLString(m_pIp);
		connection_properties["port"]          = m_Port;
		connection_properties["userName"]      = sql::SQLString(m_pUser);
		connection_properties["password"]      = sql::SQLString(m_pPass);
		connection_properties["OPT_RECONNECT"] = true;

		// Create connection
		m_pDriver = get_driver_instance();
		m_pConnection = m_pDriver->connect(connection_properties);

		// Create Statement
		m_pStatement = m_pConnection->createStatement();

		// Create database if not exists
		if(g_Config.m_SvSqlCreateTables)
		{
			str_format(aBuf, sizeof(aBuf), "CREATE DATABASE IF NOT EXISTS %s", m_pDatabase);
			m_pStatement->execute(aBuf);
		}

		// Connect to specific database
		m_pConnection->setSchema(m_pDatabase);
		dbg_msg("sql", "sql connection established");
		return true;
	}
	catch (sql::SQLException &e)
	{
		dbg_msg("sql", "MySQL ERROR: %s", e.what());
		dbg_msg("sql", "ERROR: sql connection failed");
		return false;
	}
	catch (const std::exception& ex)
	{
		// ...
		dbg_msg("sql", "1 %s",ex.what());

	}
	catch (const std::string& ex)
	{
		// ...
		dbg_msg("sql", "2 %s",ex.c_str());
	}
	catch( int )
	{
		dbg_msg("sql", "3 %s");
	}
	catch( float )
	{
		dbg_msg("sql", "4 %s");
	}

	catch( char[] )
	{
		dbg_msg("sql", "5 %s");
	}

	catch( char )
	{
		dbg_msg("sql", "6 %s");
	}
	catch (...)
	{
		dbg_msg("sql", "unknown error caused by the mysql/c++ connector, compile server_debug and use it");

		dbg_msg("sql", "ERROR: sql connection failed");
		return false;
	}
	return false;
}

void CSqlScore::Disconnect()
{
}

// create tables... should be done only once
void CSqlScore::Init()
{
	// create connection
	if(Connect())
	{
		try
		{
			char aBuf[1024];
			// create tables
			if(g_Config.m_SvSqlCreateTables)
			{
				str_format(aBuf, sizeof(aBuf), "CREATE TABLE IF NOT EXISTS %s_race (Map VARCHAR(128) BINARY NOT NULL, Name VARCHAR(%d) BINARY NOT NULL, Timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP , Time FLOAT DEFAULT 0, Server CHAR(4), cp1 FLOAT DEFAULT 0, cp2 FLOAT DEFAULT 0, cp3 FLOAT DEFAULT 0, cp4 FLOAT DEFAULT 0, cp5 FLOAT DEFAULT 0, cp6 FLOAT DEFAULT 0, cp7 FLOAT DEFAULT 0, cp8 FLOAT DEFAULT 0, cp9 FLOAT DEFAULT 0, cp10 FLOAT DEFAULT 0, cp11 FLOAT DEFAULT 0, cp12 FLOAT DEFAULT 0, cp13 FLOAT DEFAULT 0, cp14 FLOAT DEFAULT 0, cp15 FLOAT DEFAULT 0, cp16 FLOAT DEFAULT 0, cp17 FLOAT DEFAULT 0, cp18 FLOAT DEFAULT 0, cp19 FLOAT DEFAULT 0, cp20 FLOAT DEFAULT 0, cp21 FLOAT DEFAULT 0, cp22 FLOAT DEFAULT 0, cp23 FLOAT DEFAULT 0, cp24 FLOAT DEFAULT 0, cp25 FLOAT DEFAULT 0, KEY (Map, Name)) CHARACTER SET utf8 ;", m_pPrefix, MAX_NAME_LENGTH);
				m_pStatement->execute(aBuf);

				str_format(aBuf, sizeof(aBuf), "CREATE TABLE IF NOT EXISTS %s_teamrace (Map VARCHAR(128) BINARY NOT NULL, Name VARCHAR(%d) BINARY NOT NULL, Timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, Time FLOAT DEFAULT 0, ID VARBINARY(16) NOT NULL, KEY Map (Map)) CHARACTER SET utf8 ;", m_pPrefix, MAX_NAME_LENGTH);
				m_pStatement->execute(aBuf);

				str_format(aBuf, sizeof(aBuf), "CREATE TABLE IF NOT EXISTS %s_maps (Map VARCHAR(128) BINARY NOT NULL, Server VARCHAR(32) BINARY NOT NULL, Mapper VARCHAR(128) BINARY NOT NULL, Points INT DEFAULT 0, Stars INT DEFAULT 0, Timestamp TIMESTAMP, UNIQUE KEY Map (Map)) CHARACTER SET utf8 ;", m_pPrefix);
				m_pStatement->execute(aBuf);

				str_format(aBuf, sizeof(aBuf), "CREATE TABLE IF NOT EXISTS %s_saves (Savegame TEXT CHARACTER SET utf8 BINARY NOT NULL, Map VARCHAR(128) BINARY NOT NULL, Code VARCHAR(128) BINARY NOT NULL, Timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, Server CHAR(4), UNIQUE KEY (Map, Code)) CHARACTER SET utf8 ;", m_pPrefix);
				m_pStatement->execute(aBuf);

				str_format(aBuf, sizeof(aBuf), "CREATE TABLE IF NOT EXISTS %s_points (Name VARCHAR(%d) BINARY NOT NULL, Points INT DEFAULT 0, UNIQUE KEY Name (Name)) CHARACTER SET utf8 ;", m_pPrefix, MAX_NAME_LENGTH);
				m_pStatement->execute(aBuf);

				dbg_msg("sql", "tables were created successfully");
			}

			// get the best time
			str_format(aBuf, sizeof(aBuf), "SELECT Time FROM %s_race WHERE Map='%s' ORDER BY `Time` ASC LIMIT 0, 1;", m_pPrefix, m_aMap);
			m_pResults = m_pStatement->executeQuery(aBuf);

			if(m_pResults->next())
			{
				((CGameControllerDDRace*)GameServer()->m_pController)->m_CurrentRecord = (float)m_pResults->getDouble("Time");

				dbg_msg("sql", "getting best time on server done");
			}

			// delete statement
			delete m_pResults;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("sql", "MySQL ERROR: %s", e.what());
			dbg_msg("sql", "ERROR: tables were NOT created");
		}

		// disconnect from database
		Disconnect();
	}
}

void CSqlScore::CheckBirthdayThread(void *pUser)
{
	lock_wait(gs_SqlLock);

	CSqlScoreData *pData = (CSqlScoreData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			// check strings
			char originalName[MAX_NAME_LENGTH];
			strcpy(originalName,pData->m_aName);
			pData->m_pSqlData->ClearString(pData->m_aName);

			char aBuf[512];

			str_format(aBuf, sizeof(aBuf), "select year(Current) - year(Stamp) as YearsAgo from (select CURRENT_TIMESTAMP as Current, min(Timestamp) as Stamp from %s_race WHERE Name='%s') as l where dayofmonth(Current) = dayofmonth(Stamp) and month(Current) = month(Stamp) and year(Current) > year(Stamp);", pData->m_pSqlData->m_pPrefix, pData->m_aName);
			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);
			if(pData->m_pSqlData->m_pResults->next())
			{
				int yearsAgo = (int)pData->m_pSqlData->m_pResults->getInt("YearsAgo");
				str_format(aBuf, sizeof(aBuf), "Happy DDNet birthday to %s for finishing their first map %d year%s ago!", originalName, yearsAgo, yearsAgo > 1 ? "s" : "");
				pData->m_pSqlData->GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf, pData->m_ClientID);
			}

			dbg_msg("sql", "checking birthday done");

			// delete statement and results
			delete pData->m_pSqlData->m_pResults;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("sql", "MySQL ERROR: %s", e.what());
			dbg_msg("sql", "ERROR: could not check birthday");
		}

		// disconnect from database
		pData->m_pSqlData->Disconnect();
	}

	delete pData;

	lock_unlock(gs_SqlLock);
}

void CSqlScore::CheckBirthday(int ClientID)
{
	CSqlScoreData *Tmp = new CSqlScoreData();
	Tmp->m_ClientID = ClientID;
	str_copy(Tmp->m_aName, Server()->ClientName(ClientID), MAX_NAME_LENGTH);
	Tmp->m_pSqlData = this;

	void *CheckThread = thread_init(CheckBirthdayThread, Tmp);
	thread_detach(CheckThread);
}


// update stuff
void CSqlScore::LoadScoreThread(void *pUser)
{
	lock_wait(gs_SqlLock);

	CSqlScoreData *pData = (CSqlScoreData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			// check strings
			pData->m_pSqlData->ClearString(pData->m_aName);

			char aBuf[512];

			str_format(aBuf, sizeof(aBuf), "SELECT * FROM %s_race WHERE Map='%s' AND Name='%s' ORDER BY time ASC LIMIT 1;", pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_aMap, pData->m_aName);
			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);
			if(pData->m_pSqlData->m_pResults->next())
			{
				// get the best time
				float time = (float)pData->m_pSqlData->m_pResults->getDouble("Time");
				pData->m_pSqlData->PlayerData(pData->m_ClientID)->m_BestTime = time;
				pData->m_pSqlData->PlayerData(pData->m_ClientID)->m_CurrentTime = time;
				if(pData->m_pSqlData->m_pGameServer->m_apPlayers[pData->m_ClientID])
					pData->m_pSqlData->m_pGameServer->m_apPlayers[pData->m_ClientID]->m_Score = -time;

				char aColumn[8];
				if(g_Config.m_SvCheckpointSave)
				{
					for(int i = 0; i < NUM_CHECKPOINTS; i++)
					{
						str_format(aColumn, sizeof(aColumn), "cp%d", i+1);
						pData->m_pSqlData->PlayerData(pData->m_ClientID)->m_aBestCpTime[i] = (float)pData->m_pSqlData->m_pResults->getDouble(aColumn);
					}
				}
			}

			dbg_msg("sql", "getting best time done");

			// delete statement and results
			delete pData->m_pSqlData->m_pResults;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("sql", "MySQL ERROR: %s", e.what());
			dbg_msg("sql", "ERROR: could not update account");
		}

		// disconnect from database
		pData->m_pSqlData->Disconnect();
	}

	delete pData;

	lock_unlock(gs_SqlLock);
}

void CSqlScore::LoadScore(int ClientID)
{
	CSqlScoreData *Tmp = new CSqlScoreData();
	Tmp->m_ClientID = ClientID;
	str_copy(Tmp->m_aName, Server()->ClientName(ClientID), MAX_NAME_LENGTH);
	Tmp->m_pSqlData = this;

	void *LoadThread = thread_init(LoadScoreThread, Tmp);
	thread_detach(LoadThread);
}

void CSqlScore::SaveTeamScoreThread(void *pUser)
{
	lock_wait(gs_SqlLock);

	CSqlTeamScoreData *pData = (CSqlTeamScoreData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			char aBuf[2300];
			char aUpdateID[17];
			aUpdateID[0] = 0;

			for(unsigned int i = 0; i < pData->m_Size; i++)
			{
				pData->m_pSqlData->ClearString(pData->m_aNames[i]);
			}

			str_format(aBuf, sizeof(aBuf), "SELECT Name, l.ID, Time FROM ((SELECT ID FROM %s_teamrace WHERE Map = '%s' AND Name = '%s') as l) LEFT JOIN %s_teamrace as r ON l.ID = r.ID ORDER BY ID;", pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_aMap, pData->m_aNames[0], pData->m_pSqlData->m_pPrefix);
			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

			if (pData->m_pSqlData->m_pResults->rowsCount() > 0)
			{
				char aID[17];
				char aID2[17];
				char aName[64];
				unsigned int Count = 0;
				bool ValidNames = true;

				pData->m_pSqlData->m_pResults->first();
				float Time = (float)pData->m_pSqlData->m_pResults->getDouble("Time");
				strcpy(aID, pData->m_pSqlData->m_pResults->getString("ID").c_str());

				do
				{
					strcpy(aID2, pData->m_pSqlData->m_pResults->getString("ID").c_str());
					strcpy(aName, pData->m_pSqlData->m_pResults->getString("Name").c_str());
					pData->m_pSqlData->ClearString(aName);
					if (str_comp(aID, aID2) != 0)
					{
						if (ValidNames && Count == pData->m_Size)
						{
							if (pData->m_Time < Time)
								strcpy(aUpdateID, aID);
							else
								goto end;
							break;
						}

						Time = (float)pData->m_pSqlData->m_pResults->getDouble("Time");
						ValidNames = true;
						Count = 0;
						strcpy(aID, aID2);
					}

					if (!ValidNames)
						continue;

					ValidNames = false;

					for(unsigned int i = 0; i < pData->m_Size; i++)
					{
						if (str_comp(aName, pData->m_aNames[i]) == 0)
						{
							ValidNames = true;
							Count++;
							break;
						}
					}
				} while (pData->m_pSqlData->m_pResults->next());

				if (ValidNames && Count == pData->m_Size)
				{
					if (pData->m_Time < Time)
						strcpy(aUpdateID, aID);
					else
						goto end;
				}
			}

			if (aUpdateID[0])
			{
				str_format(aBuf, sizeof(aBuf), "UPDATE %s_teamrace SET Time='%.2f' WHERE ID = '%s';", pData->m_pSqlData->m_pPrefix, pData->m_Time, aUpdateID);
				dbg_msg("sql", aBuf);
				pData->m_pSqlData->m_pStatement->execute(aBuf);
			}
			else
			{
				pData->m_pSqlData->m_pStatement->execute("SET @id = UUID();");

				for(unsigned int i = 0; i < pData->m_Size; i++)
				{
				// if no entry found... create a new one
					str_format(aBuf, sizeof(aBuf), "INSERT IGNORE INTO %s_teamrace(Map, Name, Timestamp, Time, ID) VALUES ('%s', '%s', CURRENT_TIMESTAMP(), '%.2f', @id);", pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_aMap, pData->m_aNames[i], pData->m_Time);
					dbg_msg("sql", aBuf);
					pData->m_pSqlData->m_pStatement->execute(aBuf);
				}
			}

			end:
			dbg_msg("sql", "updating team time done");

			// delete results statement
			delete pData->m_pSqlData->m_pResults;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("sql", "MySQL ERROR: %s", e.what());
			dbg_msg("sql", "ERROR: could not update time");
		}

		// disconnect from database
		pData->m_pSqlData->Disconnect();
	}

	delete pData;

	lock_unlock(gs_SqlLock);
}

void CSqlScore::MapVote(int ClientID, const char* MapName)
{
	CSqlMapData *Tmp = new CSqlMapData();
	Tmp->m_ClientID = ClientID;
	str_copy(Tmp->m_aMap, MapName, 128);
	Tmp->m_pSqlData = this;

	void *VoteThread = thread_init(MapVoteThread, Tmp);
	thread_detach(VoteThread);
}

void CSqlScore::MapVoteThread(void *pUser)
{
	lock_wait(gs_SqlLock);

	CSqlMapData *pData = (CSqlMapData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		char originalMap[128];
		strcpy(originalMap,pData->m_aMap);
		pData->m_pSqlData->ClearString(pData->m_aMap);
		char clearMap[128];
		strcpy(clearMap,pData->m_aMap);
		pData->m_pSqlData->FuzzyString(pData->m_aMap);

		try
		{
			char aBuf[768];
			str_format(aBuf, sizeof(aBuf), "SELECT Map, Server FROM %s_maps WHERE Map LIKE '%s' COLLATE utf8_general_ci ORDER BY CASE WHEN Map = '%s' THEN 0 ELSE 1 END, CASE WHEN Map LIKE '%s%%' THEN 0 ELSE 1 END, LENGTH(Map), Map LIMIT 1;", pData->m_pSqlData->m_pPrefix, pData->m_aMap, clearMap, clearMap);
			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

			CPlayer *pPlayer = pData->m_pSqlData->m_pGameServer->m_apPlayers[pData->m_ClientID];

			int64 Now = pData->m_pSqlData->Server()->Tick();
			int Timeleft = 0;

			if(!pPlayer)
				goto end;

			Timeleft = pPlayer->m_LastVoteCall + pData->m_pSqlData->Server()->TickSpeed()*g_Config.m_SvVoteDelay - Now;

			if(pData->m_pSqlData->m_pResults->rowsCount() != 1)
			{
				str_format(aBuf, sizeof(aBuf), "No map like \"%s\" found. Try adding a '%%' at the start if you don't know the first character. Example: /map %%castle for \"Out of Castle\"", originalMap);
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);
			}
			else if(pPlayer->m_LastVoteCall && Timeleft > 0)
			{
				char aChatmsg[512] = {0};
				str_format(aChatmsg, sizeof(aChatmsg), "You must wait %d seconds before making another vote", (Timeleft/pData->m_pSqlData->Server()->TickSpeed())+1);
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aChatmsg);
			}
			else if(time_get() < pData->m_pSqlData->GameServer()->m_LastMapVote + (time_freq() * g_Config.m_SvVoteMapTimeDelay))
			{
				char chatmsg[512] = {0};
				str_format(chatmsg, sizeof(chatmsg), "There's a %d second delay between map-votes, please wait %d seconds.", g_Config.m_SvVoteMapTimeDelay,((pData->m_pSqlData->GameServer()->m_LastMapVote+(g_Config.m_SvVoteMapTimeDelay * time_freq()))/time_freq())-(time_get()/time_freq()));
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, chatmsg);
			}
			else
			{
				pData->m_pSqlData->m_pResults->next();
				char aMap[128];
				strcpy(aMap, pData->m_pSqlData->m_pResults->getString("Map").c_str());
				char aServer[32];
				strcpy(aServer, pData->m_pSqlData->m_pResults->getString("Server").c_str());

				for(char *p = aServer; *p; p++)
					*p = tolower(*p);

				char aCmd[256];
				str_format(aCmd, sizeof(aCmd), "sv_reset_file types/%s/flexreset.cfg; change_map \"%s\"", aServer, aMap);
				char aChatmsg[512];
				str_format(aChatmsg, sizeof(aChatmsg), "'%s' called vote to change server option '%s' (%s)", pData->m_pSqlData->GameServer()->Server()->ClientName(pData->m_ClientID), aMap, "/map");

				pData->m_pSqlData->GameServer()->m_VoteKick = false;
				pData->m_pSqlData->GameServer()->m_VoteSpec = false;
				pData->m_pSqlData->GameServer()->m_LastMapVote = time_get();
				pData->m_pSqlData->GameServer()->CallVote(pData->m_ClientID, aMap, aCmd, "/map", aChatmsg);
			}

			end:
			delete pData->m_pSqlData->m_pResults;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("sql", "MySQL ERROR: %s", e.what());
			dbg_msg("sql", "ERROR: could not update time");
		}

		pData->m_pSqlData->Disconnect();
	}

	delete pData;
	lock_unlock(gs_SqlLock);
}

void CSqlScore::MapInfo(int ClientID, const char* MapName)
{
	CSqlMapData *Tmp = new CSqlMapData();
	Tmp->m_ClientID = ClientID;
	str_copy(Tmp->m_aMap, MapName, 128);
	Tmp->m_pSqlData = this;

	void *InfoThread = thread_init(MapInfoThread, Tmp);
	thread_detach(InfoThread);
}

void CSqlScore::MapInfoThread(void *pUser)
{
	lock_wait(gs_SqlLock);

	CSqlMapData *pData = (CSqlMapData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		char originalMap[128];
		strcpy(originalMap,pData->m_aMap);
		pData->m_pSqlData->ClearString(pData->m_aMap);
		char clearMap[128];
		strcpy(clearMap,pData->m_aMap);
		pData->m_pSqlData->FuzzyString(pData->m_aMap);

		try
		{
			char aBuf[1024];
			str_format(aBuf, sizeof(aBuf), "SELECT l.Map, l.Server, Mapper, Points, Stars, (select count(Name) from %s_race where Map = l.Map) as Finishes, (select count(distinct Name) from %s_race where Map = l.Map) as Finishers, (select round(avg(Time)) from %s_race where Map = l.Map) as Average, UNIX_TIMESTAMP(l.Timestamp) as Stamp, UNIX_TIMESTAMP(CURRENT_TIMESTAMP)-UNIX_TIMESTAMP(l.Timestamp) as Ago FROM (SELECT * FROM %s_maps WHERE Map LIKE '%s' COLLATE utf8_general_ci ORDER BY CASE WHEN Map = '%s' THEN 0 ELSE 1 END, CASE WHEN Map LIKE '%s%%' THEN 0 ELSE 1 END, LENGTH(Map), Map LIMIT 1) as l;", pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_pPrefix, pData->m_aMap, clearMap, clearMap);
			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

			if(pData->m_pSqlData->m_pResults->rowsCount() != 1)
			{
				str_format(aBuf, sizeof(aBuf), "No map like \"%s\" found.", originalMap);
			}
			else
			{
				pData->m_pSqlData->m_pResults->next();
				int points = (int)pData->m_pSqlData->m_pResults->getInt("Points");
				int stars = (int)pData->m_pSqlData->m_pResults->getInt("Stars");
				int finishes = (int)pData->m_pSqlData->m_pResults->getInt("Finishes");
				int finishers = (int)pData->m_pSqlData->m_pResults->getInt("Finishers");
				int average = (int)pData->m_pSqlData->m_pResults->getInt("Average");
				char aMap[128];
				strcpy(aMap, pData->m_pSqlData->m_pResults->getString("Map").c_str());
				char aServer[32];
				strcpy(aServer, pData->m_pSqlData->m_pResults->getString("Server").c_str());
				char aMapper[128];
				strcpy(aMapper, pData->m_pSqlData->m_pResults->getString("Mapper").c_str());
				int stamp = (int)pData->m_pSqlData->m_pResults->getInt("Stamp");
				int ago = (int)pData->m_pSqlData->m_pResults->getInt("Ago");

				char pAgoString[40] = "\0";
				char pReleasedString[60] = "\0";
				if(stamp != 0)
				{
					agoTimeToString(ago, pAgoString);
					str_format(pReleasedString, sizeof(pReleasedString), ", released %s ago", pAgoString);
				}

				char pAverageString[60] = "\0";
				if(average > 0)
				{
					str_format(pAverageString, sizeof(pAverageString), " in %d:%02d average", average / 60, average % 60);
				}

				char aStars[20];
				switch(stars)
				{
					case 0: strcpy(aStars, "✰✰✰✰✰"); break;
					case 1: strcpy(aStars, "★✰✰✰✰"); break;
					case 2: strcpy(aStars, "★★✰✰✰"); break;
					case 3: strcpy(aStars, "★★★✰✰"); break;
					case 4: strcpy(aStars, "★★★★✰"); break;
					case 5: strcpy(aStars, "★★★★★"); break;
					default: aStars[0] = '\0';
				}

				str_format(aBuf, sizeof(aBuf), "\"%s\" by %s on %s (%s, %d %s, %d %s by %d %s%s%s)", aMap, aMapper, aServer, aStars, points, points == 1 ? "point" : "points", finishes, finishes == 1 ? "finish" : "finishes", finishers, finishers == 1 ? "tee" : "tees", pAverageString, pReleasedString);
			}

			pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);
			delete pData->m_pSqlData->m_pResults;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("sql", "MySQL ERROR: %s", e.what());
			dbg_msg("sql", "ERROR: could not update time");
		}

		pData->m_pSqlData->Disconnect();
	}

	delete pData;
	lock_unlock(gs_SqlLock);
}

void CSqlScore::SaveScoreThread(void *pUser)
{
	lock_wait(gs_SqlLock);

	CSqlScoreData *pData = (CSqlScoreData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			char aBuf[768];

			// check strings
			pData->m_pSqlData->ClearString(pData->m_aName);

			str_format(aBuf, sizeof(aBuf), "SELECT * FROM %s_race WHERE Map='%s' AND Name='%s' ORDER BY time ASC LIMIT 1;", pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_aMap, pData->m_aName);
			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);
			if(!pData->m_pSqlData->m_pResults->next())
			{
				delete pData->m_pSqlData->m_pResults;

				str_format(aBuf, sizeof(aBuf), "SELECT Points FROM %s_maps WHERE Map ='%s'", pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_aMap);
				pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

				if(pData->m_pSqlData->m_pResults->rowsCount() == 1)
				{
					pData->m_pSqlData->m_pResults->next();
					int points = (int)pData->m_pSqlData->m_pResults->getInt("Points");
					if (points == 1)
						str_format(aBuf, sizeof(aBuf), "You earned %d point for finishing this map!", points);
					else
						str_format(aBuf, sizeof(aBuf), "You earned %d points for finishing this map!", points);
					pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);

					str_format(aBuf, sizeof(aBuf), "INSERT INTO %s_points(Name, Points) VALUES ('%s', '%d') ON duplicate key UPDATE Name=VALUES(Name), Points=Points+VALUES(Points);", pData->m_pSqlData->m_pPrefix, pData->m_aName, points);
					pData->m_pSqlData->m_pStatement->execute(aBuf);
				}
			}

			delete pData->m_pSqlData->m_pResults;

			// if no entry found... create a new one
			str_format(aBuf, sizeof(aBuf), "INSERT IGNORE INTO %s_race(Map, Name, Timestamp, Time, Server, cp1, cp2, cp3, cp4, cp5, cp6, cp7, cp8, cp9, cp10, cp11, cp12, cp13, cp14, cp15, cp16, cp17, cp18, cp19, cp20, cp21, cp22, cp23, cp24, cp25) VALUES ('%s', '%s', CURRENT_TIMESTAMP(), '%.2f', '%s', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f');", pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_aMap, pData->m_aName, pData->m_Time, g_Config.m_SvSqlServerName, pData->m_aCpCurrent[0], pData->m_aCpCurrent[1], pData->m_aCpCurrent[2], pData->m_aCpCurrent[3], pData->m_aCpCurrent[4], pData->m_aCpCurrent[5], pData->m_aCpCurrent[6], pData->m_aCpCurrent[7], pData->m_aCpCurrent[8], pData->m_aCpCurrent[9], pData->m_aCpCurrent[10], pData->m_aCpCurrent[11], pData->m_aCpCurrent[12], pData->m_aCpCurrent[13], pData->m_aCpCurrent[14], pData->m_aCpCurrent[15], pData->m_aCpCurrent[16], pData->m_aCpCurrent[17], pData->m_aCpCurrent[18], pData->m_aCpCurrent[19], pData->m_aCpCurrent[20], pData->m_aCpCurrent[21], pData->m_aCpCurrent[22], pData->m_aCpCurrent[23], pData->m_aCpCurrent[24]);
			dbg_msg("sql", aBuf);
			pData->m_pSqlData->m_pStatement->execute(aBuf);

			dbg_msg("sql", "updating time done");
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("sql", "MySQL ERROR: %s", e.what());
			dbg_msg("sql", "ERROR: could not update time");
		}

		// disconnect from database
		pData->m_pSqlData->Disconnect();
	}

	delete pData;

	lock_unlock(gs_SqlLock);
}

void CSqlScore::SaveScore(int ClientID, float Time, float CpTime[NUM_CHECKPOINTS])
{
	CConsole* pCon = (CConsole*)GameServer()->Console();
	if(pCon->m_Cheated)
		return;
	CSqlScoreData *Tmp = new CSqlScoreData();
	Tmp->m_ClientID = ClientID;
	str_copy(Tmp->m_aName, Server()->ClientName(ClientID), MAX_NAME_LENGTH);
	Tmp->m_Time = Time;
	for(int i = 0; i < NUM_CHECKPOINTS; i++)
		Tmp->m_aCpCurrent[i] = CpTime[i];
	Tmp->m_pSqlData = this;

	void *SaveThread = thread_init(SaveScoreThread, Tmp);
	thread_detach(SaveThread);
}

void CSqlScore::SaveTeamScore(int* aClientIDs, unsigned int Size, float Time)
{
	CConsole* pCon = (CConsole*)GameServer()->Console();
	if(pCon->m_Cheated)
		return;
	CSqlTeamScoreData *Tmp = new CSqlTeamScoreData();
	for(unsigned int i = 0; i < Size; i++)
	{
		Tmp->m_aClientIDs[i] = aClientIDs[i];
		str_copy(Tmp->m_aNames[i], Server()->ClientName(aClientIDs[i]), MAX_NAME_LENGTH);
	}
	Tmp->m_Size = Size;
	Tmp->m_Time = Time;
	Tmp->m_pSqlData = this;

	void *SaveTeamThread = thread_init(SaveTeamScoreThread, Tmp);
	thread_detach(SaveTeamThread);
}

void CSqlScore::ShowTeamRankThread(void *pUser)
{
	lock_wait(gs_SqlLock);

	CSqlScoreData *pData = (CSqlScoreData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			// check strings
			char originalName[MAX_NAME_LENGTH];
			strcpy(originalName,pData->m_aName);
			pData->m_pSqlData->ClearString(pData->m_aName);

			// check sort methode
			char aBuf[600];
			char aNames[2300];
			aNames[0] = '\0';

			pData->m_pSqlData->m_pStatement->execute("SET @prev := NULL;");
			pData->m_pSqlData->m_pStatement->execute("SET @rank := 1;");
			pData->m_pSqlData->m_pStatement->execute("SET @pos := 0;");
			str_format(aBuf, sizeof(aBuf), "SELECT Rank, Name, Time FROM (SELECT Rank, l2.ID FROM ((SELECT ID, (@pos := @pos+1) pos, (@rank := IF(@prev = Time,@rank,@pos)) rank, (@prev := Time) Time FROM (SELECT ID, Time FROM %s_teamrace WHERE Map = '%s' GROUP BY ID ORDER BY Time) as ll) as l2) LEFT JOIN %s_teamrace as r2 ON l2.ID = r2.ID WHERE Map = '%s' AND Name = '%s' ORDER BY Rank LIMIT 1) as l LEFT JOIN %s_teamrace as r ON l.ID = r.ID ORDER BY Name;", pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_aMap, pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_aMap, pData->m_aName, pData->m_pSqlData->m_pPrefix);

			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

			int Rows = pData->m_pSqlData->m_pResults->rowsCount();

			if(Rows < 1)
			{
				str_format(aBuf, sizeof(aBuf), "%s has no team ranks", originalName);
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);
			}
			else
			{
				pData->m_pSqlData->m_pResults->first();

				float Time = (float)pData->m_pSqlData->m_pResults->getDouble("Time");
				int Rank = (int)pData->m_pSqlData->m_pResults->getInt("Rank");

				for(int Row = 0; Row < Rows; Row++)
				{
					strcat(aNames, pData->m_pSqlData->m_pResults->getString("Name").c_str());
					pData->m_pSqlData->m_pResults->next();

					if (Row < Rows - 2)
						strcat(aNames, ", ");
					else if (Row < Rows - 1)
						strcat(aNames, " & ");
				}

				pData->m_pSqlData->m_pResults->first();

				if(g_Config.m_SvHideScore)
				{
					str_format(aBuf, sizeof(aBuf), "Your team time: %02d:%05.02f", (int)(Time/60), Time-((int)Time/60*60));
					pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);
				}
				else
				{
					str_format(aBuf, sizeof(aBuf), "%d. %s Team time: %02d:%05.02f, requested by %s", Rank, aNames, (int)(Time/60), Time-((int)Time/60*60), pData->m_aRequestingPlayer);
					pData->m_pSqlData->GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf, pData->m_ClientID);
				}
			}

			dbg_msg("sql", "showing teamrank done");

			// delete results and statement
			delete pData->m_pSqlData->m_pResults;
		}
		catch (sql::SQLException &e)
		{
		dbg_msg("sql", "MySQL ERROR: %s", e.what());
		dbg_msg("sql", "ERROR: could not show team rank");
	}

	// disconnect from database
	pData->m_pSqlData->Disconnect();
}

delete pData;

lock_unlock(gs_SqlLock);
}

void CSqlScore::ShowTeamTop5Thread(void *pUser)
{
lock_wait(gs_SqlLock);

CSqlScoreData *pData = (CSqlScoreData *)pUser;

// Connect to database
if(pData->m_pSqlData->Connect())
{
	try
	{
		// check sort methode
		char aBuf[512];

		pData->m_pSqlData->m_pStatement->execute("SET @prev := NULL;");
		pData->m_pSqlData->m_pStatement->execute("SET @previd := NULL;");
		pData->m_pSqlData->m_pStatement->execute("SET @rank := 1;");
		pData->m_pSqlData->m_pStatement->execute("SET @pos := 0;");
		str_format(aBuf, sizeof(aBuf), "SELECT ID, Name, Time, rank FROM (SELECT r.ID, Name, rank, l.Time FROM ((SELECT ID, rank, Time FROM (SELECT ID, (@pos := IF(@previd = ID,@pos,@pos+1)) pos, (@previd := ID), (@rank := IF(@prev = Time,@rank,@pos)) rank, (@prev := Time) Time FROM (SELECT ID, MIN(Time) as Time FROM %s_teamrace WHERE Map = '%s' GROUP BY ID ORDER BY `Time` ASC) as all_top_times) as a LIMIT %d, 5) as l) LEFT JOIN %s_teamrace as r ON l.ID = r.ID ORDER BY Time ASC, r.ID, Name ASC) as a;", pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_aMap, pData->m_Num-1, pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_aMap);
		pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

		// show teamtop5
		pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "------- Team Top 5 -------");

		int Rows = pData->m_pSqlData->m_pResults->rowsCount();

		if (Rows >= 1) {
			char aID[17];
			char aID2[17];
			char aNames[2300];
			int Rank = 0;
			float Time = 0;
			int aCuts[320]; // 64 * 5
			int CutPos = 0;

			aNames[0] = '\0';
			aCuts[0] = -1;

			pData->m_pSqlData->m_pResults->first();
			strcpy(aID, pData->m_pSqlData->m_pResults->getString("ID").c_str());
			for(int Row = 0; Row < Rows; Row++)
			{
				strcpy(aID2, pData->m_pSqlData->m_pResults->getString("ID").c_str());
				if (str_comp(aID, aID2) != 0)
				{
					strcpy(aID, aID2);
					aCuts[CutPos++] = Row - 1;
				}
				pData->m_pSqlData->m_pResults->next();
			}
			aCuts[CutPos] = Rows - 1;

			CutPos = 0;
			pData->m_pSqlData->m_pResults->first();
			for(int Row = 0; Row < Rows; Row++)
			{
				strcat(aNames, pData->m_pSqlData->m_pResults->getString("Name").c_str());

				if (Row < aCuts[CutPos] - 1)
					strcat(aNames, ", ");
				else if (Row < aCuts[CutPos])
					strcat(aNames, " & ");

				Time = (float)pData->m_pSqlData->m_pResults->getDouble("Time");
				Rank = (float)pData->m_pSqlData->m_pResults->getInt("rank");

				if (Row == aCuts[CutPos])
				{
					str_format(aBuf, sizeof(aBuf), "%d. %s Team Time: %02d:%05.2f", Rank, aNames, (int)(Time/60), Time-((int)Time/60*60));
					pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);
					CutPos++;
					aNames[0] = '\0';
				}

				pData->m_pSqlData->m_pResults->next();
			}
		}

		pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "-------------------------------");

		dbg_msg("sql", "showing teamtop5 done");

		// delete results and statement
		delete pData->m_pSqlData->m_pResults;
	}
	catch (sql::SQLException &e)
	{
		dbg_msg("sql", "MySQL ERROR: %s", e.what());
		dbg_msg("sql", "ERROR: could not show teamtop5");
		}

		// disconnect from database
		pData->m_pSqlData->Disconnect();
	}

	delete pData;

	lock_unlock(gs_SqlLock);
}

void CSqlScore::ShowRankThread(void *pUser)
{
	lock_wait(gs_SqlLock);

	CSqlScoreData *pData = (CSqlScoreData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			// check strings
			char originalName[MAX_NAME_LENGTH];
			strcpy(originalName,pData->m_aName);
			pData->m_pSqlData->ClearString(pData->m_aName);

			// check sort methode
			char aBuf[600];

			pData->m_pSqlData->m_pStatement->execute("SET @prev := NULL;");
			pData->m_pSqlData->m_pStatement->execute("SET @rank := 1;");
			pData->m_pSqlData->m_pStatement->execute("SET @pos := 0;");
			str_format(aBuf, sizeof(aBuf), "SELECT Rank, Name, Time FROM (SELECT Name, (@pos := @pos+1) pos, (@rank := IF(@prev = Time,@rank, @pos)) rank, (@prev := Time) Time FROM (SELECT Name, min(Time) as Time FROM %s_race WHERE Map = '%s' GROUP BY Name ORDER BY `Time` ASC) as a) as b WHERE Name = '%s';", pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_aMap, pData->m_aName);

			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

			if(pData->m_pSqlData->m_pResults->rowsCount() != 1)
			{
				str_format(aBuf, sizeof(aBuf), "%s is not ranked", originalName);
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);
			}
			else
			{
				pData->m_pSqlData->m_pResults->next();

				float Time = (float)pData->m_pSqlData->m_pResults->getDouble("Time");
				int Rank = (int)pData->m_pSqlData->m_pResults->getInt("Rank");
				if(g_Config.m_SvHideScore)
				{
					str_format(aBuf, sizeof(aBuf), "Your time: %02d:%05.2f", (int)(Time/60), Time-((int)Time/60*60));
					pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);
				}
				else
				{
					str_format(aBuf, sizeof(aBuf), "%d. %s Time: %02d:%05.2f, requested by %s", Rank, pData->m_pSqlData->m_pResults->getString("Name").c_str(), (int)(Time/60), Time-((int)Time/60*60), pData->m_aRequestingPlayer);
					pData->m_pSqlData->GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf, pData->m_ClientID);
				}
			}

			dbg_msg("sql", "showing rank done");

			// delete results and statement
			delete pData->m_pSqlData->m_pResults;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("sql", "MySQL ERROR: %s", e.what());
			dbg_msg("sql", "ERROR: could not show rank");
		}

		// disconnect from database
		pData->m_pSqlData->Disconnect();
	}

	delete pData;

	lock_unlock(gs_SqlLock);
}

void CSqlScore::ShowTeamRank(int ClientID, const char* pName, bool Search)
{
	CSqlScoreData *Tmp = new CSqlScoreData();
	Tmp->m_ClientID = ClientID;
	str_copy(Tmp->m_aName, pName, MAX_NAME_LENGTH);
	Tmp->m_Search = Search;
	str_format(Tmp->m_aRequestingPlayer, sizeof(Tmp->m_aRequestingPlayer), "%s", Server()->ClientName(ClientID));
	Tmp->m_pSqlData = this;

	void *TeamRankThread = thread_init(ShowTeamRankThread, Tmp);
	thread_detach(TeamRankThread);
}

void CSqlScore::ShowRank(int ClientID, const char* pName, bool Search)
{
	CSqlScoreData *Tmp = new CSqlScoreData();
	Tmp->m_ClientID = ClientID;
	str_copy(Tmp->m_aName, pName, MAX_NAME_LENGTH);
	Tmp->m_Search = Search;
	str_format(Tmp->m_aRequestingPlayer, sizeof(Tmp->m_aRequestingPlayer), "%s", Server()->ClientName(ClientID));
	Tmp->m_pSqlData = this;

	void *RankThread = thread_init(ShowRankThread, Tmp);
	thread_detach(RankThread);
}

void CSqlScore::ShowTop5Thread(void *pUser)
{
	lock_wait(gs_SqlLock);

	CSqlScoreData *pData = (CSqlScoreData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			// check sort methode
			char aBuf[512];
			pData->m_pSqlData->m_pStatement->execute("SET @prev := NULL;");
			pData->m_pSqlData->m_pStatement->execute("SET @rank := 1;");
			pData->m_pSqlData->m_pStatement->execute("SET @pos := 0;");
			str_format(aBuf, sizeof(aBuf), "SELECT Name, Time, rank FROM (SELECT Name, (@pos := @pos+1) pos, (@rank := IF(@prev = Time,@rank, @pos)) rank, (@prev := Time) Time FROM (SELECT Name, min(Time) as Time FROM %s_race WHERE Map = '%s' GROUP BY Name ORDER BY `Time` ASC) as a) as b LIMIT %d, 5;", pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_aMap, pData->m_Num-1);
			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

			// show top5
			pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "----------- Top 5 -----------");

			int Rank = 0;
			float Time = 0;
			while(pData->m_pSqlData->m_pResults->next())
			{
				Time = (float)pData->m_pSqlData->m_pResults->getDouble("Time");
				Rank = (float)pData->m_pSqlData->m_pResults->getInt("rank");
				str_format(aBuf, sizeof(aBuf), "%d. %s Time: %02d:%05.2f", Rank, pData->m_pSqlData->m_pResults->getString("Name").c_str(), (int)(Time/60), Time-((int)Time/60*60));
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);
				//Rank++;
			}
			pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "-------------------------------");

			dbg_msg("sql", "showing top5 done");

			// delete results and statement
			delete pData->m_pSqlData->m_pResults;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("sql", "MySQL ERROR: %s", e.what());
			dbg_msg("sql", "ERROR: could not show top5");
		}

		// disconnect from database
		pData->m_pSqlData->Disconnect();
	}

	delete pData;

	lock_unlock(gs_SqlLock);
}

void CSqlScore::ShowTimesThread(void *pUser)
{
	lock_wait(gs_SqlLock);
	CSqlScoreData *pData = (CSqlScoreData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			char originalName[MAX_NAME_LENGTH];
			strcpy(originalName,pData->m_aName);
			pData->m_pSqlData->ClearString(pData->m_aName);

			char aBuf[512];

			if(pData->m_Search) // last 5 times of a player
				str_format(aBuf, sizeof(aBuf), "SELECT Time, UNIX_TIMESTAMP(CURRENT_TIMESTAMP)-UNIX_TIMESTAMP(Timestamp) as Ago, UNIX_TIMESTAMP(Timestamp) as Stamp FROM %s_race WHERE Map = '%s' AND Name = '%s' ORDER BY Ago ASC LIMIT %d, 5;", pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_aMap, pData->m_aName, pData->m_Num-1);
			else// last 5 times of server
				str_format(aBuf, sizeof(aBuf), "SELECT Name, Time, UNIX_TIMESTAMP(CURRENT_TIMESTAMP)-UNIX_TIMESTAMP(Timestamp) as Ago, UNIX_TIMESTAMP(Timestamp) as Stamp FROM %s_race WHERE Map = '%s' ORDER BY Ago ASC LIMIT %d, 5;", pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_aMap, pData->m_Num-1);

			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

			// show top5
			if(pData->m_pSqlData->m_pResults->rowsCount() == 0)
			{
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "There are no times in the specified range");
				goto end;
			}

			str_format(aBuf, sizeof(aBuf), "------------ Last Times No %d - %d ------------",pData->m_Num,pData->m_Num + pData->m_pSqlData->m_pResults->rowsCount() - 1);
			pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);

			float pTime = 0;
			int pSince = 0;
			int pStamp = 0;

			while(pData->m_pSqlData->m_pResults->next())
			{
				char pAgoString[40] = "\0";
				pSince = (int)pData->m_pSqlData->m_pResults->getInt("Ago");
				pStamp = (int)pData->m_pSqlData->m_pResults->getInt("Stamp");
				pTime = (float)pData->m_pSqlData->m_pResults->getDouble("Time");

				agoTimeToString(pSince,pAgoString);

				if(pData->m_Search) // last 5 times of a player
				{
					if(pStamp == 0) // stamp is 00:00:00 cause it's an old entry from old times where there where no stamps yet
						str_format(aBuf, sizeof(aBuf), "%d min %.2f sec, don't know how long ago", (int)(pTime/60), pTime-((int)pTime/60*60));
					else
						str_format(aBuf, sizeof(aBuf), "%s ago, %d min %.2f sec", pAgoString,(int)(pTime/60), pTime-((int)pTime/60*60));
				}
				else // last 5 times of the server
				{
					if(pStamp == 0) // stamp is 00:00:00 cause it's an old entry from old times where there where no stamps yet
						str_format(aBuf, sizeof(aBuf), "%s, %02d:%05.02f s, don't know when", pData->m_pSqlData->m_pResults->getString("Name").c_str(), (int)(pTime/60), pTime-((int)pTime/60*60));
					else
						str_format(aBuf, sizeof(aBuf), "%s, %s ago, %02d:%05.02f s", pData->m_pSqlData->m_pResults->getString("Name").c_str(), pAgoString, (int)(pTime/60), pTime-((int)pTime/60*60));
				}
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);
			}
			pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "----------------------------------------------------");

			dbg_msg("sql", "showing times done");

			// delete results and statement
			delete pData->m_pSqlData->m_pResults;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("sql", "MySQL ERROR: %s", e.what());
			dbg_msg("sql", "ERROR: could not show times");
		}
		end:
		// disconnect from database
		pData->m_pSqlData->Disconnect();
	}
	delete pData;

	lock_unlock(gs_SqlLock);
}

void CSqlScore::ShowTeamTop5(IConsole::IResult *pResult, int ClientID, void *pUserData, int Debut)
{
	CSqlScoreData *Tmp = new CSqlScoreData();
	Tmp->m_Num = Debut;
	Tmp->m_ClientID = ClientID;
	Tmp->m_pSqlData = this;

	void *TeamTop5Thread = thread_init(ShowTeamTop5Thread, Tmp);
	thread_detach(TeamTop5Thread);
}

void CSqlScore::ShowTop5(IConsole::IResult *pResult, int ClientID, void *pUserData, int Debut)
{
	CSqlScoreData *Tmp = new CSqlScoreData();
	Tmp->m_Num = Debut;
	Tmp->m_ClientID = ClientID;
	Tmp->m_pSqlData = this;

	void *Top5Thread = thread_init(ShowTop5Thread, Tmp);
	thread_detach(Top5Thread);
}

void CSqlScore::ShowTimes(int ClientID, int Debut)
{
	CSqlScoreData *Tmp = new CSqlScoreData();
	Tmp->m_Num = Debut;
	Tmp->m_ClientID = ClientID;
	Tmp->m_pSqlData = this;
	Tmp->m_Search = false;

	void *TimesThread = thread_init(ShowTimesThread, Tmp);
	thread_detach(TimesThread);
}

void CSqlScore::ShowTimes(int ClientID, const char* pName, int Debut)
{
	CSqlScoreData *Tmp = new CSqlScoreData();
	Tmp->m_Num = Debut;
	Tmp->m_ClientID = ClientID;
	str_copy(Tmp->m_aName, pName, MAX_NAME_LENGTH);
	Tmp->m_pSqlData = this;
	Tmp->m_Search = true;

	void *TimesThread = thread_init(ShowTimesThread, Tmp);
	thread_detach(TimesThread);
}

void CSqlScore::FuzzyString(char *pString)
{
	char newString[32*4-1];
	int pos = 0;

	for(int i=0;i<64;i++)
	{
		if(!pString[i])
			break;

		newString[pos++] = pString[i];
		if (pString[i] != '\\' && str_utf8_isstart(pString[i+1]))
			newString[pos++] = '%';
	}

	newString[pos] = '\0';
	strcpy(pString, newString);
}

// anti SQL injection
void CSqlScore::ClearString(char *pString, int size)
{
	char *newString = (char *)malloc(size * 2 - 1);
	int pos = 0;

	for(int i=0;i<size;i++)
	{
		if(pString[i] == '\\')
		{
			newString[pos++] = '\\';
			newString[pos++] = '\\';
		}
		else if(pString[i] == '\'')
		{
			newString[pos++] = '\\';
			newString[pos++] = '\'';
		}
		else if(pString[i] == '"')
		{
			newString[pos++] = '\\';
			newString[pos++] = '"';
		}
		else
		{
			newString[pos++] = pString[i];
		}
	}

	newString[pos] = '\0';

	strcpy(pString,newString);
	free(newString);
}

void CSqlScore::agoTimeToString(int agoTime, char agoString[])
{
	char aBuf[20];
	int times[7] =
	{
			60 * 60 * 24 * 365 ,
			60 * 60 * 24 * 30 ,
			60 * 60 * 24 * 7,
			60 * 60 * 24 ,
			60 * 60 ,
			60 ,
			1
	};
	char names[7][6] =
	{
			"year",
			"month",
			"week",
			"day",
			"hour",
			"min",
			"sec"
	};

	int seconds = 0;
	char name[6];
	int count = 0;
	int i = 0;

	// finding biggest match
	for(i = 0; i<7; i++)
	{
		seconds = times[i];
		strcpy(name,names[i]);

		count = floor((float)agoTime/(float)seconds);
		if(count != 0)
		{
			break;
		}
	}

	if(count == 1)
	{
		str_format(aBuf, sizeof(aBuf), "%d %s", 1 , name);
	}
	else
	{
		str_format(aBuf, sizeof(aBuf), "%d %ss", count , name);
	}
	strcat(agoString,aBuf);

	if (i + 1 < 7)
	{
		// getting second piece now
		int seconds2 = times[i+1];
		char name2[6];
		strcpy(name2,names[i+1]);

		// add second piece if it's greater than 0
		int count2 = floor((float)(agoTime - (seconds * count)) / (float)seconds2);

		if (count2 != 0)
		{
			if(count2 == 1)
			{
				str_format(aBuf, sizeof(aBuf), " and %d %s", 1 , name2);
			}
			else
			{
				str_format(aBuf, sizeof(aBuf), " and %d %ss", count2 , name2);
			}
			strcat(agoString,aBuf);
		}
	}
}

void CSqlScore::ShowPointsThread(void *pUser)
{
	lock_wait(gs_SqlLock);

	CSqlScoreData *pData = (CSqlScoreData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			// check strings
			char originalName[MAX_NAME_LENGTH];
			strcpy(originalName,pData->m_aName);
			pData->m_pSqlData->ClearString(pData->m_aName);

			pData->m_pSqlData->m_pStatement->execute("SET @prev := NULL;");
			pData->m_pSqlData->m_pStatement->execute("SET @rank := 1;");
			pData->m_pSqlData->m_pStatement->execute("SET @pos := 0;");

			char aBuf[512];
			str_format(aBuf, sizeof(aBuf), "select Rank, Name, Points from (select (@pos := @pos+1) pos, (@rank := IF(@prev = Points,@rank,@pos)) Rank, Points, Name from (select (@prev := Points) Points, Name from %s_points order by Points desc) as ll) as l where Name = '%s';", pData->m_pSqlData->m_pPrefix, pData->m_aName);
			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

			if(pData->m_pSqlData->m_pResults->rowsCount() != 1)
			{
				str_format(aBuf, sizeof(aBuf), "%s has not collected any points so far", originalName);
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);
			}
			else
			{
				pData->m_pSqlData->m_pResults->next();
				int count = (int)pData->m_pSqlData->m_pResults->getInt("Points");
				int rank = (int)pData->m_pSqlData->m_pResults->getInt("rank");
				str_format(aBuf, sizeof(aBuf), "%d. %s Points: %d, requested by %s", rank, pData->m_pSqlData->m_pResults->getString("Name").c_str(), count, pData->m_aRequestingPlayer);
				pData->m_pSqlData->GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf, pData->m_ClientID);
			}

			dbg_msg("sql", "showing points done");

			// delete results and statement
			delete pData->m_pSqlData->m_pResults;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("sql", "MySQL ERROR: %s", e.what());
			dbg_msg("sql", "ERROR: could not show points");
		}

		// disconnect from database
		pData->m_pSqlData->Disconnect();
	}

	delete pData;

	lock_unlock(gs_SqlLock);
}

void CSqlScore::ShowPoints(int ClientID, const char* pName, bool Search)
{
	CSqlScoreData *Tmp = new CSqlScoreData();
	Tmp->m_ClientID = ClientID;
	str_copy(Tmp->m_aName, pName, MAX_NAME_LENGTH);
	Tmp->m_Search = Search;
	str_format(Tmp->m_aRequestingPlayer, sizeof(Tmp->m_aRequestingPlayer), "%s", Server()->ClientName(ClientID));
	Tmp->m_pSqlData = this;

	void *PointsThread = thread_init(ShowPointsThread, Tmp);
	thread_detach(PointsThread);
}

void CSqlScore::ShowTopPointsThread(void *pUser)
{
	lock_wait(gs_SqlLock);

	CSqlScoreData *pData = (CSqlScoreData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			char aBuf[512];
			pData->m_pSqlData->m_pStatement->execute("SET @prev := NULL;");
			pData->m_pSqlData->m_pStatement->execute("SET @rank := 1;");
			pData->m_pSqlData->m_pStatement->execute("SET @pos := 0;");
			str_format(aBuf, sizeof(aBuf), "select Rank, Name, Points from (select (@pos := @pos+1) pos, (@rank := IF(@prev = Points,@rank,@pos)) Rank, Points, Name from (select (@prev := Points) Points, Name from %s_points order by Points desc) as ll) as l LIMIT %d, 5;", pData->m_pSqlData->m_pPrefix, pData->m_Num-1);

			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

			// show top points
			pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "-------- Top Points --------");

			while(pData->m_pSqlData->m_pResults->next())
			{
				str_format(aBuf, sizeof(aBuf), "%d. %s Points: %d", pData->m_pSqlData->m_pResults->getInt("rank"), pData->m_pSqlData->m_pResults->getString("Name").c_str(), pData->m_pSqlData->m_pResults->getInt("Points"));
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);
			}
			pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "-------------------------------");

			dbg_msg("sql", "showing toppoints done");

			// delete results and statement
			delete pData->m_pSqlData->m_pResults;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("sql", "MySQL ERROR: %s", e.what());
			dbg_msg("sql", "ERROR: could not show toppoints");
		}

		// disconnect from database
		pData->m_pSqlData->Disconnect();
	}

	delete pData;

	lock_unlock(gs_SqlLock);
}

void CSqlScore::ShowTopPoints(IConsole::IResult *pResult, int ClientID, void *pUserData, int Debut)
{
	CSqlScoreData *Tmp = new CSqlScoreData();
	Tmp->m_Num = Debut;
	Tmp->m_ClientID = ClientID;
	Tmp->m_pSqlData = this;

	void *TopPointsThread = thread_init(ShowTopPointsThread, Tmp);
	thread_detach(TopPointsThread);
}

void CSqlScore::RandomMapThread(void *pUser)
{
	lock_wait(gs_SqlLock);

	CSqlScoreData *pData = (CSqlScoreData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			char aBuf[512];
			if(pData->m_Num)
				str_format(aBuf, sizeof(aBuf), "select * from %s_maps where Server = \"%s\" and Stars = \"%d\" order by RAND() limit 1;", pData->m_pSqlData->m_pPrefix, g_Config.m_SvServerType, pData->m_Num);
			else
				str_format(aBuf, sizeof(aBuf), "select * from %s_maps where Server = \"%s\" order by RAND() limit 1;", pData->m_pSqlData->m_pPrefix, g_Config.m_SvServerType);
			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

			if(pData->m_pSqlData->m_pResults->rowsCount() != 1)
			{
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "No maps found on this server!");
			}
			else
			{
				pData->m_pSqlData->m_pResults->next();
				char aMap[128];
				strcpy(aMap, pData->m_pSqlData->m_pResults->getString("Map").c_str());

				str_format(aBuf, sizeof(aBuf), "change_map \"%s\"", aMap);
				pData->m_pSqlData->GameServer()->Console()->ExecuteLine(aBuf);
			}

			dbg_msg("sql", "voting random map done");

			// delete results and statement
			delete pData->m_pSqlData->m_pResults;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("sql", "MySQL ERROR: %s", e.what());
			dbg_msg("sql", "ERROR: could not vote random map");
		}

		// disconnect from database
		pData->m_pSqlData->Disconnect();
	}

	delete pData;

	lock_unlock(gs_SqlLock);
}

void CSqlScore::RandomUnfinishedMapThread(void *pUser)
{
	lock_wait(gs_SqlLock);

	CSqlScoreData *pData = (CSqlScoreData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			char originalName[MAX_NAME_LENGTH];
			strcpy(originalName,pData->m_aName);
			pData->m_pSqlData->ClearString(pData->m_aName);

			char aBuf[512];
			if(pData->m_Num)
				str_format(aBuf, sizeof(aBuf), "select * from %s_maps where Server = \"%s\" and Stars = \"%d\" and not exists (select * from %s_race where Name = \"%s\" and %s_race.Map = %s_maps.Map) order by RAND() limit 1;", pData->m_pSqlData->m_pPrefix, g_Config.m_SvServerType, pData->m_Num, pData->m_pSqlData->m_pPrefix, pData->m_aName, pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_pPrefix);
			else
				str_format(aBuf, sizeof(aBuf), "select * from %s_maps where Server = \"%s\" and not exists (select * from %s_race where Name = \"%s\" and %s_race.Map = %s_maps.Map) order by RAND() limit 1;", pData->m_pSqlData->m_pPrefix, g_Config.m_SvServerType, pData->m_pSqlData->m_pPrefix, pData->m_aName, pData->m_pSqlData->m_pPrefix, pData->m_pSqlData->m_pPrefix);
			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

			if(pData->m_pSqlData->m_pResults->rowsCount() != 1)
			{
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "You have no unfinished maps on this server!");
			}
			else
			{
				pData->m_pSqlData->m_pResults->next();
				char aMap[128];
				strcpy(aMap, pData->m_pSqlData->m_pResults->getString("Map").c_str());

				str_format(aBuf, sizeof(aBuf), "change_map \"%s\"", aMap);
				pData->m_pSqlData->GameServer()->Console()->ExecuteLine(aBuf);
			}

			dbg_msg("sql", "voting random unfinished map done");

			// delete results and statement
			delete pData->m_pSqlData->m_pResults;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("sql", "MySQL ERROR: %s", e.what());
			dbg_msg("sql", "ERROR: could not vote random unfinished map");
		}

		// disconnect from database
		pData->m_pSqlData->Disconnect();
	}

	delete pData;

	lock_unlock(gs_SqlLock);
}

void CSqlScore::RandomMap(int ClientID, int stars)
{
	CSqlScoreData *Tmp = new CSqlScoreData();
	Tmp->m_Num = stars;
	Tmp->m_ClientID = ClientID;
	str_copy(Tmp->m_aName, GameServer()->Server()->ClientName(ClientID), MAX_NAME_LENGTH);
	Tmp->m_pSqlData = this;

	void *RandomThread = thread_init(RandomMapThread, Tmp);
	thread_detach(RandomThread);
}

void CSqlScore::RandomUnfinishedMap(int ClientID, int stars)
{
	CSqlScoreData *Tmp = new CSqlScoreData();
	Tmp->m_Num = stars;
	Tmp->m_ClientID = ClientID;
	str_copy(Tmp->m_aName, GameServer()->Server()->ClientName(ClientID), MAX_NAME_LENGTH);
	Tmp->m_pSqlData = this;

	void *RandomUnfinishedThread = thread_init(RandomUnfinishedMapThread, Tmp);
	thread_detach(RandomUnfinishedThread);
}

void CSqlScore::SaveTeam(int Team, const char* Code, int ClientID, const char* Server)
{
	if((g_Config.m_SvTeam == 3 || (Team > 0 && Team < MAX_CLIENTS)) && ((CGameControllerDDRace*)(GameServer()->m_pController))->m_Teams.Count(Team) > 0)
	{
		if(((CGameControllerDDRace*)(GameServer()->m_pController))->m_Teams.GetSaving(Team))
			return;
		((CGameControllerDDRace*)(GameServer()->m_pController))->m_Teams.SetSaving(Team, true);
	}
	else
	{
		GameServer()->SendChatTarget(ClientID, "You have to be in a Team (from 1-63)");
		return;
	}

	CSqlTeamSave *Tmp = new CSqlTeamSave();
	Tmp->m_Team = Team;
	Tmp->m_ClientID = ClientID;
	str_copy(Tmp->m_Code, Code, 32);
	str_copy(Tmp->m_Server, Server, sizeof(Tmp->m_Server));
	Tmp->m_pSqlData = this;

	void *SaveThread = thread_init(SaveTeamThread, Tmp);
	thread_detach(SaveThread);
}

void CSqlScore::SaveTeamThread(void *pUser)
{
	CSaveTeam* SavedTeam = 0;
	CSqlTeamSave *pData = (CSqlTeamSave *)pUser;

	char TeamString[65536];
	int Team = pData->m_Team;
	char OriginalCode[32];
	str_copy(OriginalCode, pData->m_Code, sizeof(OriginalCode));
	pData->m_pSqlData->ClearString(pData->m_Code, sizeof(pData->m_Code));
	char Map[128];
	str_copy(Map, g_Config.m_SvMap, 128);
	pData->m_pSqlData->ClearString(Map, sizeof(Map));

	int Num = -1;

	if((g_Config.m_SvTeam == 3 || (Team > 0 && Team < MAX_CLIENTS)) && ((CGameControllerDDRace*)(pData->m_pSqlData->GameServer()->m_pController))->m_Teams.Count(Team) > 0)
	{
		SavedTeam = new CSaveTeam(pData->m_pSqlData->GameServer()->m_pController);
		Num = SavedTeam->save(Team);
		switch (Num)
		{
			case 1:
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "You have to be in a Team (from 1-63)");
				break;
			case 2:
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "Could not find your Team");
				break;
			case 3:
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "Unable to find all Characters");
				break;
			case 4:
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "Your team is not started yet");
				break;
		}
		if(!Num)
		{
			str_copy(TeamString, SavedTeam->GetString(), sizeof(TeamString));
			pData->m_pSqlData->ClearString(TeamString, sizeof(TeamString));
		}
	}
	else
		pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "You have to be in a Team (from 1-63)");

	lock_wait(gs_SqlLock);
	// Connect to database
	if(!Num && pData->m_pSqlData->Connect())
	{
		try
		{
			char aBuf[512];
			str_format(aBuf, sizeof(aBuf), "select Savegame from %s_saves where Code = '%s' and Map = '%s';",  pData->m_pSqlData->m_pPrefix, pData->m_Code, Map);
			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

			if (pData->m_pSqlData->m_pResults->rowsCount() == 0)
			{
				// delete results and statement
				delete pData->m_pSqlData->m_pResults;

				char aBuf[65536];
				str_format(aBuf, sizeof(aBuf), "INSERT IGNORE INTO %s_saves(Savegame, Map, Code, Timestamp, Server) VALUES ('%s', '%s', '%s', CURRENT_TIMESTAMP(), '%s')",  pData->m_pSqlData->m_pPrefix, TeamString, Map, pData->m_Code, pData->m_Server);
				dbg_msg("sql", aBuf);
				pData->m_pSqlData->m_pStatement->execute(aBuf);

				char aBuf2[256];
				str_format(aBuf2, sizeof(aBuf2), "Team successfully saved. Use '/load %s' to continue", OriginalCode);
				pData->m_pSqlData->GameServer()->SendChatTeam(Team, aBuf2);
				((CGameControllerDDRace*)(pData->m_pSqlData->GameServer()->m_pController))->m_Teams.KillSavedTeam(Team);
			}
			else
			{
				dbg_msg("sql", "ERROR: this save-code already exists");
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "This save-code already exists");
			}
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("sql", "MySQL ERROR: %s", e.what());
			dbg_msg("sql", "ERROR: could not save the team");
			pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "MySQL error: Could not save the team");
		}

		// disconnect from database
		pData->m_pSqlData->Disconnect();
	}
	else if(!Num)
	{
		dbg_msg("sql", "connection failed");
		pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "Error: Unable to connect to SQL-Server");
	}

	((CGameControllerDDRace*)(pData->m_pSqlData->GameServer()->m_pController))->m_Teams.SetSaving(Team, false);

	delete pData;
	if(SavedTeam)
		delete SavedTeam;

	lock_unlock(gs_SqlLock);
}

void CSqlScore::LoadTeam(const char* Code, int ClientID)
{
	CSqlTeamLoad *Tmp = new CSqlTeamLoad();
	str_copy(Tmp->m_Code, Code, 32);
	Tmp->m_ClientID = ClientID;
	Tmp->m_pSqlData = this;

	void *LoadThread = thread_init(LoadTeamThread, Tmp);
	thread_detach(LoadThread);
}

void CSqlScore::LoadTeamThread(void *pUser)
{
	CSaveTeam* SavedTeam;
	CSqlTeamLoad *pData = (CSqlTeamLoad *)pUser;

	SavedTeam = new CSaveTeam(pData->m_pSqlData->GameServer()->m_pController);

	pData->m_pSqlData->ClearString(pData->m_Code, sizeof(pData->m_Code));
	char Map[128];
	str_copy(Map, g_Config.m_SvMap, 128);
	pData->m_pSqlData->ClearString(Map, sizeof(Map));
	int Num;

	lock_wait(gs_SqlLock);
	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			char aBuf[768];
			str_format(aBuf, sizeof(aBuf), "select Savegame, Server, UNIX_TIMESTAMP(CURRENT_TIMESTAMP)-UNIX_TIMESTAMP(Timestamp) as Ago from %s_saves where Code = '%s' and Map = '%s';",  pData->m_pSqlData->m_pPrefix, pData->m_Code, Map);
			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

			if (pData->m_pSqlData->m_pResults->rowsCount() > 0)
			{
				pData->m_pSqlData->m_pResults->first();
				char ServerName[5];
				str_copy(ServerName, pData->m_pSqlData->m_pResults->getString("Server").c_str(), sizeof(ServerName));
				if(str_comp(ServerName, g_Config.m_SvSqlServerName))
				{
					str_format(aBuf, sizeof(aBuf), "You have to be on the '%s' server to load this savegame", ServerName);
					pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);
					goto end;
				}

				pData->m_pSqlData->m_pResults->getInt("Ago");
				int since = (int)pData->m_pSqlData->m_pResults->getInt("Ago");

				if(since < g_Config.m_SvSaveGamesDelay)
				{
					str_format(aBuf, sizeof(aBuf), "You have to wait %d seconds until you can load this savegame", g_Config.m_SvSaveGamesDelay - since);
					pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);
					goto end;
				}

				Num = SavedTeam->LoadString(pData->m_pSqlData->m_pResults->getString("Savegame").c_str());

				if(Num)
					pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "Unable to load savegame: data corrupted");
				else
				{

					bool found = false;
					for (int i = 0; i < SavedTeam->GetMembersCount(); i++)
					{
						if(str_comp(SavedTeam->SavedTees[i].GetName(), pData->m_pSqlData->Server()->ClientName(pData->m_ClientID)) == 0)
						{ found = true; break; }
					}
					if (!found)
						pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "You don't belong to this team");
					else
					{

						int n;
						for(n = 1; n<64; n++)
						{
							if(((CGameControllerDDRace*)(pData->m_pSqlData->GameServer()->m_pController))->m_Teams.Count(n) == 0)
								break;
						}

						if(((CGameControllerDDRace*)(pData->m_pSqlData->GameServer()->m_pController))->m_Teams.Count(n) > 0)
						{
							n = ((CGameControllerDDRace*)(pData->m_pSqlData->GameServer()->m_pController))->m_Teams.m_Core.Team(pData->m_ClientID); // if all Teams are full your the only one in your team
						}

						Num = SavedTeam->load(n);

						if(Num == 1)
						{
							pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "You have to be in a team (from 1-63)");
						}
						else if(Num >= 10 && Num < 100)
						{
							char aBuf[256];
							str_format(aBuf, sizeof(aBuf), "Unable to find player: '%s'", SavedTeam->SavedTees[Num-10].GetName());
							pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);
						}
						else if(Num >= 100)
						{
							char aBuf[256];
							str_format(aBuf, sizeof(aBuf), "%s is racing right now, Team can't be loaded if a Tee is racing already", SavedTeam->SavedTees[Num-100].GetName());
							pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);
						}
						else
						{
							pData->m_pSqlData->GameServer()->SendChatTeam(n, "Loading successfully done");
							char aBuf[512];
							str_format(aBuf, sizeof(aBuf), "DELETE from %s_saves where Code='%s' and Map='%s';", pData->m_pSqlData->m_pPrefix, pData->m_Code, Map);
							pData->m_pSqlData->m_pStatement->execute(aBuf);
						}
					}
				}
			}
			else
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "No such savegame for this map");
			end:
			// delete results and statement
			delete pData->m_pSqlData->m_pResults;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("sql", "MySQL ERROR: %s", e.what());
			dbg_msg("sql", "ERROR: could not load the team");
			pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "MySQL error: Could not load the team");
		}

		// disconnect from database
		pData->m_pSqlData->Disconnect();
	}
	else
	{
		dbg_msg("sql", "connection failed");
		pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "Error: Unable to connect to sql server");
	}

	delete pData;
	delete SavedTeam;

	lock_unlock(gs_SqlLock);
}

#endif
