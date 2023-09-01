#include <wayfire/per-output-plugin.hpp>
#include <wayfire/view.hpp>
#include <wayfire/matcher.hpp>
#include <wayfire/workspace-set.hpp>
#include <wayfire/output.hpp>
#include <wayfire/signal-definitions.hpp>
#include <wayfire/txn/transaction-manager.hpp>

#include "firedecor-subsurface.hpp"
#include "firedecor-theme.hpp"
#include "wayfire/core.hpp"
#include "wayfire/signal-provider.hpp"
#include "wayfire/toplevel-view.hpp"
#include "wayfire/toplevel.hpp"

#include <stdio.h>

class wayfire_firedecor_t : public wf::plugin_interface_t {
    wf::view_matcher_t ignore_views{"firedecor/ignore_views"};
    wf::option_wrapper_t<std::string> extra_themes{"firedecor/extra_themes"};
    wf::config::config_manager_t& config = wf::get_core().config;

    wf::signal::connection_t<wf::txn::new_transaction_signal> on_new_tx = [this] (wf::txn::new_transaction_signal *ev) {
        // For each transaction, we need to consider what happens with participating views
        for (const auto& obj : ev->tx->get_objects() ) {
            if ( auto toplevel = std::dynamic_pointer_cast<wf::toplevel_t>( obj ) ) {
                /**
                 * First check whether the toplevel already has decoration
                 * In that case, we should just set the correct margins
                 */
                if ( auto deco = toplevel->get_data<wf::firedecor::simple_decorator_t>() ) {
                    toplevel->pending().margins = deco->get_margins( toplevel->pending() );
                    continue;
                }

                /**
                 * Second case: the view is already mapped, or the transaction does not map it.
                 * The view is not being decorated, so nothing to do here.
                 */
                if ( toplevel->current().mapped || !toplevel->pending().mapped ) {
                    continue;
                }

                /** Third case: the transaction will map the toplevel. */
                auto view = wf::find_view_for_toplevel( toplevel );
                wf::dassert( view != nullptr, "Mapping a toplevel means there must be a corresponding view!" );

                if ( should_decorate_view( view ) ) {
                    update_view_decoration(view);
                }
            }
        }
    };

public:
    bool ignore_decoration_of_view( wayfire_view view ) {
        return ignore_views.matches( view );
    }

    bool should_decorate_view( wayfire_toplevel_view view ) {
        return view->should_be_decorated() && !ignore_decoration_of_view( view );
    }

    void update_view_decoration(wayfire_view view) {
        if (auto toplevel = wf::toplevel_cast(view)) {
            if (should_decorate_view(toplevel)) {
                adjust_new_decorations( toplevel );
            } else {
                remove_decoration(toplevel);
            }
        }
    }

    void adjust_new_decorations( wayfire_toplevel_view view ) {
        auto toplevel = view->toplevel();

        toplevel->store_data( std::make_unique<wf::firedecor::simple_decorator_t>( view ) );
        auto  deco    = toplevel->get_data<wf::firedecor::simple_decorator_t>();
        auto& pending = toplevel->pending();

        pending.margins = deco->get_margins( pending );

        if ( !pending.fullscreen && !pending.tiled_edges ) {
            pending.geometry = wf::expand_geometry_by_margins( pending.geometry, pending.margins );
        }
    }

    void remove_decoration( wayfire_toplevel_view view ) {
        view->toplevel()->erase_data<wf::firedecor::simple_decorator_t>();
        auto& pending = view->toplevel()->pending();

        if ( !pending.fullscreen && !pending.tiled_edges ) {
            pending.geometry = wf::shrink_geometry_by_margins( pending.geometry, pending.margins );
        }

        pending.margins = { 0, 0, 0, 0 };
    }

    wf::signal::connection_t<wf::view_decoration_state_updated_signal> on_decoration_state_updated =
        [this] (wf::view_decoration_state_updated_signal *ev) {
            update_view_decoration( ev->view );
        };

public:
    void init() override {
        wf::get_core().connect( &on_decoration_state_updated );
        wf::get_core().tx_manager->connect( &on_new_tx );

        for (auto& view : wf::get_core().get_all_views()) {
            update_view_decoration(view);
        }
    }

    void fini() override {
        for (auto view : wf::get_core().get_all_views()) {
            if ( auto toplevel = wf::toplevel_cast(view)) {
                remove_decoration(toplevel);
                wf::get_core().tx_manager->schedule_object(toplevel->toplevel());
            }
        }
    }
};

DECLARE_WAYFIRE_PLUGIN(wayfire_firedecor_t);
