#include "RoundRobinCodisPool.h"
#include "Log.h"
#include "json/json.h"
#include "ScopedLock.h"

using namespace bfd::codis;

static pthread_t bns_check_tid = -1;
static bool RoundRobinCodisPool::bns_check_run = false;
static bool bns_empty = false;
RoundRobinCodisPool::RoundRobinCodisPool(const string& proxyPath, const string& businessID)
				:m_ProxyPath(proxyPath),m_Mutex(PTHREAD_MUTEX_INITIALIZER),
				 proxyIndex(-1),m_BusinessID(businessID)
{
	Init();
}

RoundRobinCodisPool::~RoundRobinCodisPool()
{
    bns_check_run = false;
    pthread_join(bns_check_tid, NULL);
    ScopedLock lock(m_Mutex);
    //std::lock_guard<std::mutex> lck(m_Mutex);
	for (size_t i=0; i<m_Proxys.size(); i++)
	{
		if (m_Proxys[i] != NULL)
		{
			delete m_Proxys[i];
			m_Proxys[i] = NULL;
		}
	}
}

CodisClient* RoundRobinCodisPool::GetProxy()
{
    ScopedLock lock(m_Mutex);
    //std::lock_guard<std::mutex> lck(m_Mutex);
    int idx = ++proxyIndex % m_Proxys.size();
    return m_Proxys[idx];
}

void RoundRobinCodisPool::Init()
{
	vector<pair<string, int> > proxyInfos;

	proxyInfos = GetProxyInfos();
	if (proxyInfos.size() == 0)
	{
		LOG(ERROR, "no proxy can be used!");
		exit(1);
	}
	InitProxyConns(proxyInfos);

    // bns check
    int ret = pthread_create(&bns_check_tid, NULL, &BnsCheck, this);
    if (ret != 0) {
        stringstream stream;
        stream << "Create pthread error!";
        LOG(ERROR, stream.str());
        exit(1);
    }
}

vector<pair<string, int> > RoundRobinCodisPool::GetProxyInfos()
{
    vector<pair<string, int> > proxys;
    char buf[1024];   
    char cmd[1024]={0};   
    FILE *ptr;   
    snprintf(cmd, 1024, "get_instance_by_service %s -ips", m_ProxyPath.c_str());
    //printf("bns: %s\n", m_ProxyPath.c_str());
    if ((ptr = popen(cmd, "r")) != NULL)   
    {   
        while (fgets(buf, 1024, ptr) != NULL)   
        {
            vector<string> buf_vec = split(string(buf), ' ');
            if (buf_vec.size() < 3) {
                LOG(ERROR, string("buf_vec size is wrong, buf is") + buf);
            }
            string ip_ = buf_vec[1];
            int port_ = atoi(buf_vec[2].c_str());
            int status_ = atoi(buf_vec[3].c_str());
            //printf("ip:%s port:%d\n", ip_.c_str(), port_);
            if (status_ == 0) {
                proxys.push_back(make_pair(ip_, port_));
            }
        }
        pclose(ptr);   
        ptr = NULL;   
    }   
    else  
    {
        printf("popen %s error\n", cmd);
    }
    return proxys;
}

void RoundRobinCodisPool::InitProxyConns(const vector<pair<string, int> >& proxyInfos)
{
	vector<CodisClient*> proxys;
	for (size_t i=0; i<proxyInfos.size(); i++)
	{
		CodisClient *proxy = new CodisClient(proxyInfos[i].first, proxyInfos[i].second, m_BusinessID);
		proxys.push_back(proxy);
	}
	{
		ScopedLock lock(m_Mutex);
        //std::lock_guard<std::mutex> lck (m_Mutex);
		m_Proxys.swap(proxys);
	}
	for (size_t i=0; i<proxys.size(); i++)
	{
		if (proxys[i] != NULL)
		{
			delete proxys[i];
			proxys[i] = NULL;
		}
	}
}

void *RoundRobinCodisPool::BnsCheck(void *arg) {
    RoundRobinCodisPool* ptr = reinterpret_cast<RoundRobinCodisPool*>(arg);
    bns_check_run = true;
    while(bns_check_run) {
        vector<pair<string, int> > proxyInfos;
        proxyInfos = ptr->GetProxyInfos();
        //printf("bns size:%d, redis size:%d\n", proxyInfos.size(), ptr->m_Proxys.size());
        if (proxyInfos.size() > 0 && (bns_empty || proxyInfos.size() != ptr->m_Proxys.size())) {
            ptr->InitProxyConns(proxyInfos);
            bns_empty = false;
        } else if (proxyInfos.size() == 0) {
            bns_empty = true;
        }
        sleep(10);
    }
}
