#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <cmath>
#include "standalone_utils.h"

using namespace std;
namespace msutils {
/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

#if defined(_WIN32)
#include <windows.h>

#include <psapi.h>
#pragma comment(lib, "psapi.lib")

#elif defined(__unix__) || defined(__unix) || defined(unix) || \
        (defined(__APPLE__) && defined(__MACH__))

#include <sys/resource.h>
#include <unistd.h>

#if defined(__APPLE__) && defined(__MACH__)

#include <mach/mach.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) || \
        (defined(__sun__) || defined(__sun) ||    \
         defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || \
        defined(__gnu_linux__)
#include <stdio.h>

#endif

#else
#error "Cannot define getPeakRSS( ) or getCurrentRSS( ) for an unknown OS."
#endif

/**
 * Returns the peak (maximum so far) resident set size (physical
 * memory use) measured in bytes, or zero if the value cannot be
 * determined on this OS.
 */
size_t getPeakRSS() {
#if defined(_WIN32)
    /* Windows -------------------------------------------------- */
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
    return (size_t)info.PeakWorkingSetSize;

#elif (defined(_AIX) || defined(__TOS__AIX__)) || \
        (defined(__sun__) || defined(__sun) ||    \
         defined(sun) && (defined(__SVR4) || defined(__svr4__)))
    /* AIX and Solaris ------------------------------------------ */
    struct psinfo psinfo;
    int fd = -1;
    if ((fd = open("/proc/self/psinfo", O_RDONLY)) == -1)
        return (size_t)0L; /* Can't open? */
    if (read(fd, &psinfo, sizeof(psinfo)) != sizeof(psinfo)) {
        close(fd);
        return (size_t)0L; /* Can't read? */
    }
    close(fd);
    return (size_t)(psinfo.pr_rssize * 1024L);

#elif defined(__unix__) || defined(__unix) || defined(unix) || \
        (defined(__APPLE__) && defined(__MACH__))
    /* BSD, Linux, and OSX -------------------------------------- */
    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);
#if defined(__APPLE__) && defined(__MACH__)
    return (size_t)rusage.ru_maxrss;
#else
    return (size_t)(rusage.ru_maxrss * 1024L);
#endif

#else
    /* Unknown OS ----------------------------------------------- */
    return (size_t)0L; /* Unsupported. */
#endif
}

/**
 * Returns the current resident set size (physical memory use) measured
 * in bytes, or zero if the value cannot be determined on this OS.
 */
size_t getCurrentRSS() {
#if defined(_WIN32)
    /* Windows -------------------------------------------------- */
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
    return (size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
    /* OSX ------------------------------------------------------ */
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info,
                  &infoCount) != KERN_SUCCESS)
        return (size_t)0L; /* Can't access? */
    return (size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || \
        defined(__gnu_linux__)
    /* Linux ---------------------------------------------------- */
    long rss = 0L;
    FILE* fp = NULL;
    if ((fp = fopen("/proc/self/statm", "r")) == NULL)
        return (size_t)0L; /* Can't open? */
    if (fscanf(fp, "%*s%ld", &rss) != 1) {
        fclose(fp);
        return (size_t)0L; /* Can't read? */
    }
    fclose(fp);
    return (size_t)rss * (size_t)sysconf(_SC_PAGESIZE);

#else
    /* AIX, BSD, Solaris, and Unknown OS ------------------------ */
    return (size_t)0L; /* Unsupported. */
#endif
}

void check(bool condition, const char* msg) {
    if (!condition) {
        printf("%s\n", msg);
        exit(1);
    }
}

void read_data_from_file(const char* fname, size_t type_size,
                         size_t& feature_num, size_t& feature_length,
                         std::vector<uint8_t>& buffer) {
    int tmp;
    size_t count;

    FILE* fin = fopen(fname, "rb");
    check(fin != nullptr, "failed to read file");

    count = fread(&tmp, 4, 1, fin);
    check(count == 1, "file format incorrect");
    feature_num = (size_t)tmp;

    count = fread(&tmp, 4, 1, fin);
    check(count == 1, "file format incorrect");
    feature_length = (size_t)tmp;

    size_t data_size = feature_num * feature_length * type_size;
    buffer.resize(data_size);
    count = fread(buffer.data(), sizeof(uint8_t), data_size, fin);
    check(count == data_size, "data file: number not match");

    fclose(fin);
}
#if 0
void print(const MS_SEARCH_RESULT_S* res, size_t num,
           size_t num_start) {
    uint64_t invalid_id = std::numeric_limits<uint64_t>::max();
    if (num_start == 0)
        printf("==megsearchv2==\n");
    size_t top_k = res->k;
    for (size_t t = 0; t < num; ++t) {
        printf("---%d---\n", (int)(t + num_start));
        for (size_t i = 0; i < top_k; ++i) {
            size_t idx = t * top_k + i;
            if (res->ids[idx] == invalid_id)
                break;
            printf("[%d] %d %.4f\n", (int)i, (int)res->ids[idx],
                   res->scores[idx]);
        }
    }
}
#endif
BatchReader::BatchReader(const char* fname, size_t batch_num,
                         size_t type_size) {
     //int tmp;
    //size_t count;

    m_type_size = type_size;

    file.open(fname, ios::in);
    if (!file.is_open())
    {
        printf("open file err!\n");
        return;
    }
        

    int linenum = 0;
    std::string strLine;
    while(getline(file, strLine))
    {
        if (strLine.empty())
            continue;
        linenum++;
    }
    file.clear();
    file.seekg(0);
    //count = fread(&tmp, 4, 1, fin);
    //check(count == 1, "file format incorrect");
    m_feature_num = linenum;//(size_t)tmp;
    printf("fname %s, linenum %d\n", fname, linenum);

                         //count = fread(&tmp, 4, 1, fin);
                         //check(count == 1, "file format incorrect");
    m_feature_len = 192;//(size_t)tmp;

    m_batch_num = batch_num;
    m_read_total = 0;
    m_unit_size = m_feature_len * m_type_size;
}
BatchReader::~BatchReader() {
    file.close();
}

size_t BatchReader::get_feature_len() {
    return m_feature_len;
}

size_t BatchReader::get_feature_num() {
    return m_feature_num;
}
size_t BatchReader::read_next_batch(std::vector<uint8_t>& buffer, size_t type_size) {
    if (m_read_total == m_feature_num)
        return 0;
    size_t count = 0;
    size_t c_count = 0;
    size_t cur_batch_num = std::min(m_batch_num, m_feature_num - m_read_total);
    buffer.resize(cur_batch_num * m_unit_size);

    std::string strLine;
    while (getline(file, strLine))
    {
        if (strLine.empty())
            continue;
        //cout << "file: "<< strLine << "\n";
        regex ws_re(" ");
        vector<string> v(sregex_token_iterator(strLine.begin(), strLine.end(), ws_re, -1), sregex_token_iterator());
        int subcount = 0;
        for (auto &&s : v)
        {
            float f = atof(s.c_str());
            if(type_size == sizeof(float))
            {
                uint8_t* c = (uint8_t*)(&f);
                buffer[c_count++] = *c++;
                buffer[c_count++] = *c++;
                buffer[c_count++] = *c++;
                buffer[c_count++] = *c++;
            }
            else
            {
                float quan = 381.448012f;
                buffer[c_count++] = (uint8_t)round(f*quan-1+128);
                //("%d ", (uint8_t)round(f*quan-1+128));
            }
            
            if(++subcount >= 192)
                break;
            //cout <<"f: " <<f << "\n";
            //cout << "s: "<<s << "\n";
        }
        //printf("\n");
        //cout << strLine << endl;
        if (++count == cur_batch_num)
            break;
        
    }
    //count = fread(buffer.data(), 1, cur_batch_num * m_unit_size, fin);
    check(count == cur_batch_num, "data file: number not match");

    m_read_total += cur_batch_num;

    return cur_batch_num;
}
size_t BatchReader::read_rest(std::vector<uint8_t>& buffer, size_t type_size) {
     size_t rest = m_feature_num - m_read_total;
    buffer.resize(rest * m_unit_size);
    size_t count = 0;
    size_t c_count = 0;
    std::string strLine;
    while (getline(file, strLine))
    {
        if (strLine.empty())
            continue;
        regex ws_re(" ");
        vector<string> v(sregex_token_iterator(strLine.begin(), strLine.end(), ws_re, -1), sregex_token_iterator());
        int subcount = 0;
        for (auto &&s : v)
        {
            float f = atof(s.c_str());
            if(type_size == sizeof(float))
            {
                uint8_t* c = (uint8_t*)(&f);
                buffer[c_count++] = *c++;
                buffer[c_count++] = *c++;
                buffer[c_count++] = *c++;
                buffer[c_count++] = *c++;
            }
            else
            {
                float quan = 381.448012f;
                buffer[c_count++] = (uint8_t)round(f*quan-1+128);
                //printf("%d ", (uint8_t)round(f*quan-1+128));
            }

            if(++subcount >= 192)
                break;
            //cout << f << "\n";
            //cout << s << "\n";
        }
        //printf("\n");
        //cout << strLine << endl;
        if (++count == rest)
            break;
    }

    check(count == rest, "data file: number not match");
    m_read_total += rest;
    return rest;
}
#if 0
MS_FEAT_SET_S read_feature_group_settings(const char* settings_file) {
    printf("dd %s\n", settings_file);
    std::ifstream ifs(settings_file);
    std::string line;
    std::unordered_map<std::string, std::string> kvs;
    while (ifs.good()) {
        line.clear();
        std::getline(ifs, line);
        if (!line.empty()) {
            auto pos = line.find('=');
            if (pos != std::string::npos) {
                kvs[line.substr(0, pos)] = line.substr(pos + 1);
                printf("%s=%s\n", line.substr(0, pos).c_str(), line.substr(pos+1).c_str());
            }
        }
    }

    MS_FEAT_SET_S settings = {};
    auto it = kvs.find("dev_type");
    if (it != kvs.end()) {
        if (it->second.front() == 'C') {
            settings.dev_type = MS_CPU;
        } else if (it->second.front() == 'G') {
            settings.dev_type = MS_GPU_CUDA;
        } else if (it->second.front() == 'O') {
            settings.dev_type = MS_GPU_OPENCL;
        } else {
            check(0, "unknown device");
        }
    }

    it = kvs.find("feature_size");
    if (it != kvs.end()) {
        int v = std::stoi(it->second);
        settings.feature_size = (size_t)v;
        printf("feature_size %d\n",
                            (int)settings.feature_size);
    }

    it = kvs.find("tag_size");
    if (it != kvs.end()) {
        int v = std::stoi(it->second);
        settings.tag_size = (size_t)v;
    }

    it = kvs.find("alpha");
    if (it != kvs.end()) {
        auto v = std::stof(it->second);
        settings.alpha = v;
    }

    it = kvs.find("beta");
    if (it != kvs.end()) {
        auto v = std::stof(it->second);
        settings.beta = v;
    }

    it = kvs.find("type");
    if (it != kvs.end()) {
        auto& v = it->second;
        if (v == "MS_FEAT_INT32") {
            settings.type = MS_FEAT_INT32;
        } else if (v == "MS_FEAT_INT8") {
            //printf("Error: Unknown type!");
            //exit(1);
            settings.type = MS_FEAT_INT8;
        } else {
            printf("Error: Unknown type!");
            exit(1);
        }
    }

    return settings;
}
#endif
}  // namespace msutils
