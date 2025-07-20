
#include "table_section.h"

using namespace Linker;

template class Linker::TableSection<Word<uint32_t>>;
template class Linker::TableSection<Word<uint64_t>>;

