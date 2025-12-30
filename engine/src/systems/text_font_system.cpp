#include "text_style_system.h"
#include "resources/resources_types.inl"
#include "core/logger.h"
#include "core/cememory.h"

#include "systems/resource_system.h"
#include "systems/texture_system.h"
#include "systems/material_system.h"
#include "text_font_system.h"

namespace caliope {

	typedef struct text_font_reference {
		text_font text_font;
		uint reference_count;
	} text_font_reference;


	typedef struct text_style_system_state {
		std::unordered_map<std::string, text_font_reference> registered_fonts;

	} text_style_system_state;

	static std::unique_ptr<text_style_system_state> state_ptr;

	bool load_text_font(text_font_resource_data& text_font_config, uint font_size);
	void destroy_text_font(text_font& tf);

	bool text_font_system_initialize()
	{
		state_ptr = std::make_unique<text_style_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		CE_LOG_INFO("Text font system initialized.");
		return true;
	}

	void text_font_system_shutdown()
	{
		state_ptr->registered_fonts.clear(); // Destroys all the text styles and their pointers.
		state_ptr.reset();
		state_ptr = nullptr;
	}

	text_font* text_font_system_adquire_font(std::string& name, uint font_size)
	{
		std::string full_name = name + "_" + std::to_string(font_size);
		if (state_ptr->registered_fonts.find(full_name) == state_ptr->registered_fonts.end()) {

			resource r;
			if (!resource_system_load(name, RESOURCE_TYPE_TEXT_FONT, r)) {
				CE_LOG_ERROR("text_font_system_adquire_font couldnt load file text style");
				return nullptr;
			}
			text_font_resource_data text_font_config = std::any_cast<text_font_resource_data>(r.data);


			if (!load_text_font(text_font_config, font_size)) {
				CE_LOG_ERROR("text_font_system_adquire_font couldnt adquire text style");
				return nullptr;
			}

			resource_system_unload(r);
		}

		state_ptr->registered_fonts[full_name].reference_count++;
		return &state_ptr->registered_fonts[full_name].text_font;
	}

	text_font_glyph* text_font_system_get_glyph(text_font* font, uint codepoint)
	{
		// TODO: Try to make a direct access (O(1)) by using the codepoint as index take into account the UTF-8 characters
		for (uint i = 0; i < font->glyphs.size(); ++i) {
			if (font->glyphs[i].codepoint == codepoint) {
				return &font->glyphs[i];
			}
		}
		return &font->glyphs[0]; // Returns -1
	}

	void text_font_system_release(std::string& name)
	{
		if (state_ptr->registered_fonts.find(name) != state_ptr->registered_fonts.end()) {

			state_ptr->registered_fonts[name].reference_count++;

			if (state_ptr->registered_fonts[name].reference_count <= 0) {
				destroy_text_font(state_ptr->registered_fonts[name].text_font);
				state_ptr->registered_fonts.erase(name);
			}
		}
	}


	bool load_text_font(text_font_resource_data& text_font_config, uint font_size) {
		text_font_reference tfr;
		
		tfr.text_font.name = std::string(&text_font_config.name[0]);
		tfr.text_font.name.append("_" + std::to_string(font_size));
		tfr.text_font.atlas_size = {1024, 1024}; // TODO: Make it dynamic according to the used number of glyphs
		

		texture* writeable_atlas = texture_system_adquire_writeable("__" + tfr.text_font.name + "_tex__", tfr.text_font.atlas_size.x, tfr.text_font.atlas_size.y, 4, true);
		material_resource_data mat_config;
		zero_memory(&mat_config, sizeof(material_resource_data));

		copy_memory(&mat_config.name, ("__" + tfr.text_font.name + "_mat__").c_str(), sizeof(char) * MAX_NAME_LENGTH);// TODO: as a pointer?
		copy_memory(&mat_config.diffuse_texture_name, writeable_atlas->name.c_str(), sizeof(char) * MAX_NAME_LENGTH); // TODO: as a pointer?
		mat_config.diffuse_color = glm::vec3(1.0f);
		mat_config.shader_name = { "Builtin.UIShader" };// TODO: Make it configurable? and as a pointer?
		

		// Obtain line height metric
		float scale_pixel_height = stbtt_ScaleForPixelHeight(&text_font_config.stb_font_info, (float)font_size);
		int ascent;
		int descent;
		int line_gap;
		stbtt_GetFontVMetrics(&text_font_config.stb_font_info, &ascent, &descent, &line_gap);
		tfr.text_font.line_height = (ascent - descent + line_gap) * scale_pixel_height;


		// TODO: From this point it can be moved to other function for future regeneration of the atlas if needs new characters
		// Insert codepoints ids
		// Defaults codepoints from ascii 32-127, plus -1 for unknown
		tfr.text_font.codepoints.resize(96, -1);
		for (uint i = 1; i < 95; ++i) {
			tfr.text_font.codepoints[i] = i + 31;
		}

		// Create the atlas single channel image
		stbtt_pack_context stbtt_context;
		std::vector<uchar> pixels;
		pixels.resize(tfr.text_font.atlas_size.x * tfr.text_font.atlas_size.y * sizeof(uchar));
		if (!stbtt_PackBegin(&stbtt_context, pixels.data(), tfr.text_font.atlas_size.x, tfr.text_font.atlas_size.y, 0, 1, 0)) {
			CE_LOG_ERROR("stbtt_PackBegin failed");
			return false;
		}



		std::vector<stbtt_packedchar>packed_chars;
		packed_chars.resize(tfr.text_font.codepoints.size());
		stbtt_pack_range range;
		range.first_unicode_codepoint_in_range = 0;
		range.font_size = font_size;
		range.num_chars = tfr.text_font.codepoints.size();
		range.chardata_for_range = packed_chars.data();
		range.array_of_unicode_codepoints = tfr.text_font.codepoints.data();
		if (!stbtt_PackFontRanges(&stbtt_context, text_font_config.binary_data.data(), 0, &range, 1)) {
			CE_LOG_ERROR("stbtt_PackFontRanges failed");
			return false;
		}

		stbtt_PackEnd(&stbtt_context);

		// Transform single-channel to RGBA
		uint pack_image_size = tfr.text_font.atlas_size.x * tfr.text_font.atlas_size.y * sizeof(uchar);
		std::vector<uchar> rgba_pixels;
		rgba_pixels.resize(pack_image_size * 4);
		for (uint i = 0; i < pack_image_size; ++i) {
			rgba_pixels[(i * 4) + 0] = pixels[i];
			rgba_pixels[(i * 4) + 1] = pixels[i];
			rgba_pixels[(i * 4) + 2] = pixels[i];
			rgba_pixels[(i * 4) + 3] = pixels[i];
		}


		// Regenerate glyphs data
		tfr.text_font.glyphs.clear();
		tfr.text_font.glyphs.resize(tfr.text_font.codepoints.size());

		for (uint16 i = 0; i < tfr.text_font.glyphs.size(); ++i) {
			stbtt_packedchar* pc = &packed_chars[i];
			text_font_glyph* g = &tfr.text_font.glyphs[i];
			g->codepoint = tfr.text_font.codepoints[i];
			g->x_offset = pc->xoff;
			g->y_offset = pc->yoff;
			g->y_offset2 = pc->yoff2;
			g->x = pc->x0;  // xmin;
			g->y = pc->y0;
			g->width = pc->x1 - pc->x0;
			g->height = pc->y1 - pc->y0;
			g->x_advance = pc->xadvance;
			g->kerning_index = stbtt_FindGlyphIndex(&text_font_config.stb_font_info, g->codepoint);

			if (g->codepoint == ' ') {
				tfr.text_font.x_advance_space = g->x_advance;
				tfr.text_font.x_advance_tab = g->x_advance * 4;
			}
		}

		// Regenerate kerning data
		tfr.text_font.kernings.resize(stbtt_GetKerningTableLength(&text_font_config.stb_font_info));
		std::vector<stbtt_kerningentry> kerning_table; // TODO: If the kerning is too much large it can occupies all the call stack?
		kerning_table.resize(tfr.text_font.kernings.size());
		uint entry_count = stbtt_GetKerningTable(&text_font_config.stb_font_info, kerning_table.data(), tfr.text_font.kernings.size());
		for (uint i = 0; i < tfr.text_font.kernings.size(); ++i) {
			text_font_kerning* k = &tfr.text_font.kernings[i];
			k->codepoint1 = kerning_table[i].glyph1;
			k->codepoint2 = kerning_table[i].glyph2;
			k->advance = (kerning_table[i].advance * scale_pixel_height) / font_size;
		}
		// TODO: End move this to other function

		// This must be at the end other wise the stb_font_info is filled with trash data
		tfr.text_font.atlas_material = material_system_adquire_from_config(mat_config);
		tfr.text_font.atlas_material->diffuse_texture = writeable_atlas;
		// Write data to atlas
		texture_system_write_data(*tfr.text_font.atlas_material->diffuse_texture, 0, pack_image_size * 4, rgba_pixels.data());

		state_ptr->registered_fonts.insert({ tfr.text_font.name, tfr });

		return true;
	}

	void destroy_text_font(text_font& tf) {
		state_ptr->registered_fonts.erase(tf.name);
		tf.name = "";
		material_system_release(tf.atlas_material->name);
		// TODO:
	}
}