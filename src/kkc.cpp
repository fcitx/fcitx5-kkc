#include "kkc.h"
namespace fcitx {
class KKCState : public InputContextProperty {
public:
    KKCState(KKC *parent)
        : parent_(parent),
          context_(kkc_context_new(parent->model()), &g_object_unref) {}

    KKC *parent_;
    std : unique_ptr<KkcContext, decltype(&g_object_unref)> context_;
};
KKC::KKC(Instance *instance)
    : instance_(instance),
      factory_([this](InputContext &) { return new KKCState(this); }),
      model_(nullptr, &g_object_unref) {
#if !GLIB_CHECK_VERSION(2, 36, 0)
    g_type_init();
#endif
    kkc_init();

    model_.reset(kkc_language_model_load("sorted3", NULL));
    loadDictionary();
    loadRule();

    reloadConfig();
    instance_->inputContextManager().registerProperty("kkcState", &factory_);
}

KKC ::~KKC() {}
void KKC::activate(const InputMethodEntry &entry, InputContextEvent &event) {}
void KKC::keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) {}
void KKC::reloadConfig() {}
void KKC::reset(const InputMethodEntry &entry, InputContextEvent &event) {}
void KKC::save() {}

void KKC::updateUI(InputContext *inputContext) {}

std::string KKC::subMode(const InputMethodEntry &, InputContext &) {}

KKCState *KKC::state(InputContext *ic) {}

} // namespace fcitx
FCITX_ADDON_FACTORY(fcitx::KKCFactory);
