#include "ChnavtectGlobal.h "
////////////////////////////////////////////////////////////
//无锁模型定义
////////////////////////////////////////////////////////////
CLockNone::CLockNone(void)
{
	m_nLockCnt = 0;
}

CLockNone::~CLockNone(void)
{
	
}

////////////////////////////////////////////////////////////
//无锁模型Lock操作
////////////////////////////////////////////////////////////
void CLockNone::Lock  ()
{
	m_nLockCnt++;
}

////////////////////////////////////////////////////////////
//无锁模型解除操作
////////////////////////////////////////////////////////////
void CLockNone::Unlock()
{
	m_nLockCnt--;
}

////////////////////////////////////////////////////////////
//获得锁计数
////////////////////////////////////////////////////////////
int  CLockNone::GetLockCnt()const
{
	return m_nLockCnt;
}

////////////////////////////////////////////////////////////
//互斥锁模型定义
////////////////////////////////////////////////////////////
CLockCriticalSection::CLockCriticalSection(void)
{
	m_nLockCnt = 0;
}

CLockCriticalSection::~CLockCriticalSection(void)
{
	
}

////////////////////////////////////////////////////////////
//互斥模型Lock操作
////////////////////////////////////////////////////////////
void CLockCriticalSection::Lock  ()
{
	m_cs.Lock();
	
	m_nLockCnt++;
}

////////////////////////////////////////////////////////////
//互斥模型Unlock操作
////////////////////////////////////////////////////////////
void CLockCriticalSection::Unlock()
{
	m_nLockCnt--;

	m_cs.Unlock();	
}

////////////////////////////////////////////////////////////
//获得锁计数
//返回值:锁计数
////////////////////////////////////////////////////////////
int  CLockCriticalSection::GetLockCnt()const 
{
	return m_nLockCnt;
}
	
////////////////////////////////////////////////////////////
//通用消息定义
////////////////////////////////////////////////////////////
IMsgData::IMsgData(void)
{
	
}

////////////////////////////////////////////////////////////
//析构函数定义
////////////////////////////////////////////////////////////
IMsgData::~IMsgData(void)
{
	
}

////////////////////////////////////////////////////////////
//通用连接接口定义
////////////////////////////////////////////////////////////
IConnection::~IConnection(void)
{

}

////////////////////////////////////////////////////////////
//构造函数定义
////////////////////////////////////////////////////////////
 IConnection::IConnection(void)
{

}

////////////////////////////////////////////////////////////
///观察者定义
 ////////////////////////////////////////////////////////////
 CObserver::CObserver()
{
   
}

CObserver::~CObserver()
{
  
}

////////////////////////////////////////////////////////////
//Observer定义
BOOL CObserver::Update(IMsgData *pInfo, UINT uInfoID,  WPARAM wPara,LPARAM lPara,int nMsgLen)
{     
      return FALSE;//无实现
}

//构造函数
////////////////////////////////////////////////////////////
CUiObserver::CUiObserver()
{
	m_hWnd    = NULL;//
	m_dwMsgId = -1;
}

CUiObserver::~CUiObserver()
{
	
}
////////////////////////////////////////////////////////////
//设置通知信息
////////////////////////////////////////////////////////////
void CUiObserver::SetNotifyInfo(HWND hWnd,DWORD dwMsgId)
{
	m_hWnd    = hWnd;//
	m_dwMsgId = dwMsgId;//变量
}

////////////////////////////////////////////////////////////
//消息更新操作
//pInfo ,消息
//uInfoID，消息ID
//wPara。附加参数
//lPara。附加参数
//nMsgLen：消息长度
////////////////////////////////////////////////////////////
BOOL CUiObserver::Update(IMsgData *pInfo, UINT uInfoID, WPARAM wPara,LPARAM lPara,int nMsgLen)
{   	
	AUTOLOCK(m_csList)
	
	BOOL bUpdate = FALSE;
	if(pInfo == NULL)
		return FALSE;
	//入口判断
	if(m_hWnd != NULL && pInfo != NULL)
	{   //发送消息
		PostMessage(m_hWnd,m_dwMsgId,(WPARAM)pInfo,lPara);
		bUpdate = TRUE;
	}		
	
	return bUpdate;
}

////////////////////////////////////////////////////////////
///自由观察者定义
////////////////////////////////////////////////////////////
CFreeObserver::CFreeObserver()
{
	
}

////////////////////////////////////////////////////////////
//析构函数
////////////////////////////////////////////////////////////
CFreeObserver::~CFreeObserver()
{
	
}

////////////////////////////////////////////////////////////
//自由观察者消息更新操作
//pInfo ,消息
//uInfoID，消息ID
//wPara。附加参数
//lPara。附加参数
//nMsgLen：消息长度
////////////////////////////////////////////////////////////
BOOL CFreeObserver::Update(IMsgData *pInfo, UINT uInfoID, WPARAM wPara,LPARAM lPara,int nMsgLen)
{
	AUTOLOCK(m_csUpdate)
	BOOL bUodate = FALSE;
	
	return bUodate;
}

////////////////////////////////////////////////////////////
///被观察者定义
////////////////////////////////////////////////////////////
CObservedObj::CObservedObj()
{
	
}

CObservedObj::~CObservedObj()
{
    
}

////////////////////////////////////////////////////////////
//增加观察者调用函数
//pObserver:观察者
////////////////////////////////////////////////////////////
void CObservedObj::Attatch(CObserver *pObserver)
{    
	AUTOLOCK(m_csObservers);
	//遍历
	list<CObserver *>::iterator it = m_listObserver.begin();
	for(; it !=  m_listObserver.end() ; it++)
	{
		if(pObserver == *it)
			break;
	}
	 
	//增加
	if(it == m_listObserver.end())
	{
		m_listObserver.push_back(pObserver);
	}
}

////////////////////////////////////////////////////////////
//减少观察者调用函数
//pObserver:观察者
////////////////////////////////////////////////////////////
void CObservedObj::Detatch(CObserver *pObserver)
{
	AUTOLOCK(m_csObservers);

	if(pObserver == NULL || m_listObserver.size() == 0)
		return ;
	//遍历
	list<CObserver *>::iterator it = m_listObserver.begin();
	for(; it !=  m_listObserver.end() ; it++)
	{
		if(pObserver == *it)
			break;
	}
	
	//减少
	if(it != m_listObserver.end())
	{
		m_listObserver.erase(it);
	}
}

////////////////////////////////////////////////////////////
//消息通知函数
//pMsg ，消息
//nMsgType ，消息类型
//wParam，附加参数
//lParam，附加参数
//nLen.长度
////////////////////////////////////////////////////////////
BOOL CObservedObj::Notify(IMsgData *pMsg,int nMsgType,WPARAM wParam,LPARAM lParam,int nLen)
{
	AUTOLOCK(m_csObservers);

	CObserver * pObserver = NULL;
	BOOL        bUpdate   = FALSE;	
	//有效判断
	if(pMsg == NULL || m_listObserver.size() == 0)
		return FALSE;
	//遍历
	list<CObserver *>::iterator it = m_listObserver.begin();
	for(; it !=  m_listObserver.end() ; it++)
	{   //更新操作
		pObserver = *it;
		if(pObserver != NULL)
		{   
			bUpdate = pObserver->Update(pMsg->Copy(),nMsgType,wParam,lParam,nLen);
		}
	}
	
	pMsg->Release();//释放
	return bUpdate; 	
}

////////////////////////////////////////////////////////////
//管道定义
////////////////////////////////////////////////////////////
CPipe::CPipe(void)
{
	
}

////////////////////////////////////////////////////////////
//析构函数
////////////////////////////////////////////////////////////
CPipe::~CPipe(void)
{
	
}

int CMessager::Advise(CSinker* pSinker)
{
	size_t i = 0 ;

	if(pSinker == NULL) return m_sinkers.size();

	for(;i < m_sinkers.size(); i++){
		if(m_sinkers[i] == pSinker)
			break;
	}

	if(i == m_sinkers.size())
		m_sinkers.push_back(pSinker);

	return m_sinkers.size();
}

int CMessager::Unadvise(const CSinker* pSinker)
{
	size_t i = 0 ;

	if(pSinker == NULL) return m_sinkers.size();

	for(;i < m_sinkers.size(); i++){
		if(m_sinkers[i] == pSinker)
			break;
	}

	if(i != m_sinkers.size())
		m_sinkers.erase(m_sinkers.begin()+i);

	return m_sinkers.size();

}

void CMessager::NotifyMsg(int msgid, char* msg ,int msglen)
{	
	for(size_t i = 0 ; i < m_sinkers.size(); i++){
		m_sinkers[i]->Update(msgid,msg,msglen);
	}
}