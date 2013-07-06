#pragma once

#include <glib.h>
#include <string>

template <typename T, typename unref_res_t, void (*unref_res)(unref_res_t *)>
class ResourceWrapper {
public:
	ResourceWrapper(T *p = nullptr) : p_(p) {}
	~ResourceWrapper() { free_resource(); }
    ResourceWrapper(const ResourceWrapper&) = delete;
    ResourceWrapper& operator=(const ResourceWrapper&) = delete;
	T *operator->() const { return p_; }
	bool operator!() const { return p_ == nullptr; }

	void reset(T *newp) {
		if (p_ != newp) {
			free_resource();
			p_ = newp;
		}
	}

	friend inline T *get_impl(const ResourceWrapper& rw) {
		return rw.p_; 
	}

	friend inline T **get_addr(ResourceWrapper& rw) {
		return &rw.p_; 
	}
private:
	T *p_;

	void free_resource() { if (p_) unref_res(p_); }

// Helper for enabling 'if (sp)'
	struct Tester {
        Tester() {}
    private:
        void operator delete(void*);
    };
public:
	// enable 'if (sp)'
    operator Tester*() const {
        if (!*this)
            return 0;
        static Tester t;
        return &t;
    }
};

namespace glib {
	typedef ResourceWrapper<gchar, void, g_free> CharStr;
	typedef ResourceWrapper<GError, GError, g_error_free> Error;
}


extern char *locale_to_utf8(const char *locstr);
extern std::string utf8_to_locale_ign_err(const std::string& utf8_str);

