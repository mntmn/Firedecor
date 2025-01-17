#include <wayfire/opengl.hpp>

#include "firedecor-buttons.hpp"
#include "firedecor-theme.hpp"

#define HOVERED  1.0
#define NORMAL   0.0
#define PRESSED -0.7

namespace wf {
    namespace firedecor {
        button_t::button_t(const decoration_theme_t& t, std::function<void()> damage) :
            theme(t), damage_callback(damage) {
            this->texture_dirty = true;
        }

        void button_t::set_button_type(button_type_t type) {
            this->type = type;
            this->hover.animate(0, 0);
            add_idle_damage();
        }

        button_type_t button_t::get_button_type() const {
            return this->type;
        }

        void button_t::set_active(bool active) {
            if (this->active != active) {
                this->active = active;
                add_idle_damage();
            }
        }

        void button_t::set_maximized(uint32_t edges) {
            /*if (this->maximized != (edges == wf::TILED_EDGES_ALL)) {
              this->maximized = (edges == wf::TILED_EDGES_ALL);
              add_idle_damage();
              }*/
        }

        void button_t::set_hover(bool is_hovered) {
            this->is_hovered = is_hovered;
            if (!this->is_pressed) {
                if (is_hovered) {
                    this->hover.animate(HOVERED);
                } else {
                    this->hover.animate(NORMAL);
                }
            }

            add_idle_damage();
        }

        /**
         * Set whether the button is pressed or not.
         * Affects appearance.
         */
        void button_t::set_pressed(bool is_pressed) {
            this->is_pressed = is_pressed;
            if (is_pressed) {
                this->hover.animate(PRESSED);
            } else {
                this->hover.animate(is_hovered ? HOVERED : NORMAL);
            }

            add_idle_damage();
        }

        void button_t::render(const wf::render_target_t& fb, wf::geometry_t geometry,
                              wf::geometry_t scissor) {

            if (this->texture_dirty) {
                update_texture(fb.scale);
            }

            OpenGL::render_begin(fb);
            fb.logic_scissor(scissor);
            OpenGL::render_texture(button_texture.tex, fb, geometry, {1, 1, 1, 1},
                                   OpenGL::TEXTURE_TRANSFORM_INVERT_Y);
            OpenGL::render_end();

            if (this->hover.running()) {
                add_idle_damage();
            }
        }

        void button_t::update_texture(double scale) {
            auto surface = theme.form_button(type, hover, active, maximized, scale);
            OpenGL::render_begin();
            cairo_surface_upload_to_texture(surface, this->button_texture);
            OpenGL::render_end();
            cairo_surface_destroy(surface);
            this->texture_dirty = false;
        }

        void button_t::add_idle_damage() {
            this->idle_damage.run_once([=, this] () {
                this->damage_callback();
                this->texture_dirty = true;
            });
        }
    }
}
