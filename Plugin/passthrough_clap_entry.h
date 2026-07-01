#pragma once

extern bool passthrough_entry_init(const char *plugin_path);
extern void passthrough_entry_deinit(void);
extern const void *passthrough_entry_get_factory(const char *factory_id);
