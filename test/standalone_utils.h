#pragma once

#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <regex>
using namespace std;

namespace msutils {

size_t getPeakRSS();

size_t getCurrentRSS();

// if !condition print message
void check(bool condition, const char* msg);

// read all data at once
void read_data_from_file(const char* fname, size_t type_size,
                         size_t& feature_num, size_t& feature_length,
                         std::vector<uint8_t>& buffer);
//void print(const MS_SEARCH_RESULT_S* res, size_t num,
//#           size_t start_num = 0);

class BatchReader {
private:
    size_t m_feature_len;
    size_t m_feature_num;
    size_t m_batch_num;
    size_t m_read_total;
    size_t m_type_size;
    size_t m_unit_size;
    std::ifstream file;

public:
    BatchReader(const char* fname, size_t batch_num, size_t type_size);

    ~BatchReader();

    size_t get_feature_len();

    size_t get_feature_num();

    // number of features, return 0 at EOF
    size_t read_next_batch(std::vector<uint8_t>& buffer, size_t type_size);

    size_t read_rest(std::vector<uint8_t>& buffer, size_t type_size);
};

//MS_FEAT_SET_S read_feature_group_settings(const char* settings_file);

}  // namespace msutils
