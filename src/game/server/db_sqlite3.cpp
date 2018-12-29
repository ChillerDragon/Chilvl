#include "db_sqlite3.h"
#include <base/system.h> //ChillerDragon needs for str_len func
#include <fstream> //ChillerDragon much wow shitty filestream usage -.-
#include <string> //ChillerDragon ikr no kewl C style stuff

bool CQuery::Next()
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	int Ret = sqlite3_step(m_pStatement);
	return Ret == SQLITE_ROW;
}

void CQuery::Query(CSql *pDatabase, char *pQuery)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
	if (!pQuery) {
		dbg_msg("SQLite","[WARNING] no pQuery found in CQuery::Query()");
	}
#endif
	m_pDatabase = pDatabase;
	m_pDatabase->Query(this, pQuery);
}

void CQuery::OnData()
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	Next();
}

int CQuery::GetID(const char *pName)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	for (int i = 0; i < GetColumnCount(); i++)
	{
		if (str_comp(GetName(i), pName) == 0)
			return i;
	}
	return -1;
}

void CSql::WorkerThread()
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	while (m_Running)
	{
		lock_wait(m_Lock); //lock queue
		if (m_lpQueries.size() > 0)
		{
			CQuery *pQuery = m_lpQueries.front();
			m_lpQueries.pop();
			lock_unlock(m_Lock); //unlock queue

			int Ret;
			Ret = sqlite3_prepare_v2(m_pDB, pQuery->m_Query.c_str(), -1, &pQuery->m_pStatement, 0);
			if (Ret == SQLITE_OK)
			{
				if (!m_Running) //last check
					break;

                // pQuery->OnData()

                lock_wait(m_CallbackLock);
                m_lpExecutedQueries.push(pQuery);
                lock_unlock(m_CallbackLock);

				// sqlite3_finalize(pQuery->m_pStatement);
			}
			else
			{
				dbg_msg("SQLite", "WorkerError: %s", sqlite3_errmsg(m_pDB));
			}

			// delete pQuery;
		}
		else
		{
			thread_sleep(100);
			lock_unlock(m_Lock); //unlock queue
		}

		thread_sleep(10);
	}
}

void CSql::Tick()
{
    while(true) {
        lock_wait(m_CallbackLock);
        if(!m_lpExecutedQueries.size()) {
            lock_unlock(m_CallbackLock);
            break;
        }

        CQuery *pQuery = m_lpExecutedQueries.front();
        m_lpExecutedQueries.pop();

        pQuery->OnData();

        sqlite3_finalize(pQuery->m_pStatement);

        lock_unlock(m_CallbackLock);
        delete pQuery;
    }
}

void CSql::InitWorker(void *pUser)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	CSql *pSelf = (CSql *)pUser;
	pSelf->WorkerThread();
}

CQuery *CSql::Query(CQuery *pQuery, std::string QueryString)
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
	if (!pQuery) {
		dbg_msg("SQLite", "[WARNING] no pQuery found in CQuery *CSql::Query(CQuery *pQuery, std::string QueryString)");
		return NULL;
	}
#endif

	pQuery->m_Query = QueryString;

	lock_wait(m_Lock);
	m_lpQueries.push(pQuery);
	lock_unlock(m_Lock);

	return pQuery;
}

CSql::CSql()
{
	sqlite3 *test;

	std::ifstream f;
	std::string aUserInputLoL;

	f.open("database_path.txt", std::ios::in);
	if (getline(f, aUserInputLoL))
		dbg_msg("SQLite", "successfully loaded path '%s'", aUserInputLoL.c_str());
	else
		dbg_msg("SQLite","error reading database pathfile 'database_path.txt'");
	f.close();
	dbg_msg("SQLite", "connecting to '%s' ", aUserInputLoL.c_str());
	int rc = sqlite3_open(aUserInputLoL.c_str(), &m_pDB);
	if (rc)
	{
		dbg_msg("SQLite", "Can't open database error: %d", rc);
		sqlite3_close(m_pDB);
		//return; //causes broken db idk why it said "no such column: UseSpawnWeapons" i have no idea why it picked this one and not the first or last
	}

	char *Query = "CREATE TABLE IF NOT EXISTS Accounts (\n\
		ID							INTEGER			PRIMARY KEY		AUTOINCREMENT,\n\
		Username					VARCHAR(32)		NOT NULL,\n\
		Password					VARCHAR(128)	NOT NULL,\n\
		RegisterDate				VARCHAR(32)		DEFAULT '',\n\
		IsLoggedIn					INTEGER			DEFAULT 0,\n\
		LastLoginPort				INTEGER			DEFAULT 0,\n\
		LastLogoutIGN1				VARCHAR(32)		DEFAULT '',\n\
		LastLogoutIGN2				VARCHAR(32)		DEFAULT '',\n\
		LastLogoutIGN3				VARCHAR(32)		DEFAULT '',\n\
		LastLogoutIGN4				VARCHAR(32)		DEFAULT '',\n\
		LastLogoutIGN5				VARCHAR(32)		DEFAULT '',\n\
		IP_1						VARCHAR(32)		DEFAULT '',\n\
		IP_2						VARCHAR(32)		DEFAULT '',\n\
		IP_3						VARCHAR(32)		DEFAULT '',\n\
		Clan1						VARCHAR(32)		DEFAULT '',\n\
		Clan2						VARCHAR(32)		DEFAULT '',\n\
		Clan3						VARCHAR(32)		DEFAULT '',\n\
		Skin						VARCHAR(32)		DEFAULT '',\n\
		Level						INTEGER			DEFAULT 0,\n\
		Coins						INTEGER			DEFAULT 0,\n\
		Exp							INTEGER			DEFAULT 0,\n\
		JailTime					INTEGER			DEFAULT 0,\n\
		Hammer						INTEGER			DEFAULT 0,\n\
		Gun							INTEGER			DEFAULT 0,\n\
		Shotgun						INTEGER			DEFAULT 0,\n\
		Grenade						INTEGER			DEFAULT 0,\n\
		Rifle						INTEGER			DEFAULT 0,\n\
		Life						INTEGER			DEFAULT 0);";


	sqlite3_exec(m_pDB, Query, 0, 0, 0);

	m_Lock = lock_create();
	m_CallbackLock = lock_create();
	m_Running = true;
	thread_init(InitWorker, this);
}

CSql::~CSql()
{
#if defined(CONF_DEBUG)
	CALL_STACK_ADD();
#endif
	m_Running = false;
	lock_wait(m_Lock);

	while (m_lpQueries.size())
	{
		CQuery *pQuery = m_lpQueries.front();
		m_lpQueries.pop();
		delete pQuery;
	}

	lock_unlock(m_Lock);
	lock_destroy(m_Lock);

	lock_wait(m_CallbackLock);

	while (m_lpExecutedQueries.size())
	{
		CQuery *pQuery = m_lpExecutedQueries.front();
		m_lpExecutedQueries.pop();
		delete pQuery;
	}

    lock_unlock(m_CallbackLock);
    lock_destroy(m_CallbackLock);
}
