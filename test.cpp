
#include <map>
#include <string>
#include <thread>
#include <mutex>

#include <fstream>
#include <iostream>
#include <string>

// This class is a thread safe cache. Access to the data by SearchValue. Adding new records by AddValue.
// Mutexes protects map. 
class SomeCache
{
public:
        SomeCache()
        {
                m_mpAddProtector = new std::mutex;
                m_mpSearchProtector = new std::mutex;
        }
        ~SomeCache()
        {
                if (m_mpAddProtector) delete m_mpAddProtector;
                if (m_mpSearchProtector) delete m_mpSearchProtector;
        }

        void AddValue(std::string strKey, std::string strValue)
        {
                std::lock_guard<std::mutex> locker(*m_mpAddProtector);
                std::string* ptrResult = SearchValue(strKey);

                // insert only if it is not already in the map.
                if (ptrResult != NULL && !ptrResult->empty())
                {
                        m_mpValues.insert(std::pair<std::string, std::string>(strKey, strValue));
                }
        }

        std::string* SearchValue(std::string strKey)
        {
                std::lock_guard<std::mutex>  locker(*m_mpSearchProtector);
                std::string strSearchedValue = "";

                for (auto& elem : m_mpValues)
                {
                        if (elem.first == strKey)
                        {
                                strSearchedValue = elem.second;
                        }
                }

                std::string* strResult = &strSearchedValue;

                return strResult;
        }

        std::map<std::string, std::string> m_mpValues;
        std::mutex* m_mpAddProtector;
        std::mutex* m_mpSearchProtector;
};
SomeCache* ptrCache = NULL;

void Worker(std::string strFileToWork)
{
        std::cout << "Worker start with file " << strFileToWork << "\n";
        std::ifstream fileStream;
        fileStream.open(strFileToWork);
        
        std::string strCurrent;
        unsigned int iCounter = 0;

        while (std::getline(fileStream, strCurrent))
        {
                std::string* strValueInCache = NULL;
                strValueInCache = ptrCache->SearchValue(strCurrent);
                if (!strValueInCache && strValueInCache->size() > 0)
                {
                        iCounter++;
                }
                // just to make it a bit slow to work not that fast.
                std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        fileStream.close();
        std::cout << " File " << strFileToWork << " contains " << iCounter << " records
from cache\n";
}

class CThreadCacheUpdater
{

public:
        void Run()
        {
                m_bStopped = false;
                m_strCacheFile = "C:\\projects\\SomeFile.txt";
                while (!m_bStopped)
                {
                        UpdateCache();
                        std::this_thread::sleep_for(std::chrono::microseconds(200));
                }
        }

        void UpdateCache()
        {
                std::ifstream fileStream;
                fileStream.open(m_strCacheFile);

                std::string strKey;
                std::string strValue;
                std::string strCurrent;

                SomeCache* ptrNewCache = new SomeCache();
                while (std::getline(fileStream, strCurrent))
                {

                        // first line is a key, second is a value
                        if (strKey.empty())
                        {
                                strKey = strCurrent;
                        }
                        else 
                        {
                                strValue = strCurrent;
                                ptrNewCache->AddValue(strKey, strValue);
                                strKey = "";
                        }
                }
                fileStream.close();
                
                //replace old cache with a new fresh updated.
                SomeCache* ptrOldCache = ptrCache;
                ptrCache = ptrNewCache;
        }

        std::string m_strCacheFile;
        bool  m_bStopped;
};

int main(int argc, char* argv[])
{
        ptrCache = new SomeCache();
        // cache must have some hard coded values. Let's insert them
        ptrCache->m_mpValues.insert(std::pair<std::string, std::string>("one", "AAAA"));
        ptrCache->m_mpValues.insert(std::pair<std::string, std::string>("two", "AAAA1"));
        ptrCache->m_mpValues.insert(std::pair<std::string, std::string>("three", "AAAA2"));
        ptrCache->m_mpValues.insert(std::pair<std::string, std::string>("four", "AAAA3"));
        ptrCache->m_mpValues.insert(std::pair<std::string, std::string>("five", "bbb"));

        CThreadCacheUpdater updater;
        std::thread updaterThread(&CThreadCacheUpdater::Run, updater);

        std::thread workerThread1(Worker, "c:\\projects\\worker1.txt");
        std::thread workerThread2(Worker, "c:\\projects\\worker2.txt");
        std::thread workerThread3(Worker, "c:\\projects\\worker3.txt");


        workerThread1.join();
        workerThread2.join();
        workerThread3.join();

        updater.m_bStopped = true;
        updaterThread.join();

        if (ptrCache->m_mpAddProtector)                delete ptrCache->m_mpAddProtector;
        if (ptrCache->m_mpSearchProtector)  delete ptrCache->m_mpSearchProtector;
        if (ptrCache) delete ptrCache;
}