/**
 * @file
 * @brief
 */

#ifndef CODIS_CLIENT_ROUNDROBINCODISPOOL_H
#define CODIS_CLIENT_ROUNDROBINCODISPOOL_H

#include "CodisClient.h"
#include <vector>
//#include <mutex>

using namespace std;

namespace bfd
{
namespace codis
{

class RoundRobinCodisPool
{
public:
	RoundRobinCodisPool(const string& proxyPath, const string& businessID);
	~RoundRobinCodisPool();

	CodisClient* GetProxy();
    static bool bns_check_run;
private:
	vector<CodisClient*> m_Proxys;
	int proxyIndex;
	string m_ProxyPath;
	string m_BusinessID;
	pthread_mutex_t m_Mutex;
    //std::mutex m_Mutex;
private:
	void Init();
	vector<pair<string, int> > GetProxyInfos();
	void InitProxyConns(const vector<pair<string, int> >& proxyInfos);
    static void *BnsCheck(void *arg);
};

}
}
#endif
