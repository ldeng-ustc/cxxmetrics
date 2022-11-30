#include "cxxmetrics/metrics.h"
/**
 * @brief 
 * Use a struct to pass string literal as template parameter
 * https://ctrpeach.io/posts/cpp20-string-literal-template-parameters/
 * @tparam N 
 */
template<size_t N>
struct StringLiteral {
    constexpr StringLiteral(const char (&str)[N]) {
        std::copy_n(str, N, value);
    }
    constexpr static size_t len = N;
    char value[N];
};

static size_t id_counter = 0;
static std::vector<std::string_view> name_vec;

template<StringLiteral STR>
static size_t alloc_id() {
    auto id = id_counter ++;
    name_vec.emplace_back(STR.value, STR.len);
    return id;
}

/**
 * @brief 
 * the method use runtime static variable, from
 * https://stackoverflow.com/questions/26570904/how-can-i-generate-dense-unique-type-ids-at-compile-time
 * 
 * No runtime string process
 * @return * 
 */
template<StringLiteral STR>
size_t id() {
    static size_t id = alloc_id<STR>();
    return id;
}

std::string_view name(size_t id) {
    return name_vec[id];
}