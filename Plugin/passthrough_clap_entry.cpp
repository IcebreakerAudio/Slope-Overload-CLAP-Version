#include <clap/clap.h>
#include <cstring>

#include "passthrough_clap_entry.h"

extern "C"
{
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#endif

    const CLAP_EXPORT struct clap_plugin_entry clap_entry = {CLAP_VERSION, passthrough_entry_init,
                                                               passthrough_entry_deinit,
                                                               passthrough_entry_get_factory};

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}
