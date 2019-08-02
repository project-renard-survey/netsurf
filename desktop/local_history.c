/*
 * Copyright 2017 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 * Local history viewer implementation
 */

#include <stdlib.h>
#include <string.h>

#include "utils/errors.h"
#include "utils/nsurl.h"
#include "netsurf/types.h"
#include "netsurf/layout.h"
#include "netsurf/browser_window.h"
#include "netsurf/core_window.h"
#include "netsurf/plotters.h"
#include "netsurf/keypress.h"

#include "desktop/gui_internal.h"
#include "desktop/system_colour.h"
#include "desktop/browser_private.h"
#include "desktop/browser_history.h"
#include "desktop/local_history.h"

/**
 * local history viewer context
 */
struct local_history_session {
	struct browser_window *bw;
	struct core_window_callback_table *cw_t;
	void *core_window_handle;
	struct history_entry *cursor;
};


/**
 * plot style for drawing lines between nodes
 */
static plot_style_t pstyle_line = {
	.stroke_type = PLOT_OP_TYPE_SOLID,
	.stroke_width = plot_style_int_to_fixed(2),
};


/**
 * plot style for drawing background
 */
static plot_style_t pstyle_bg = {
	.fill_type = PLOT_OP_TYPE_SOLID,
};


/**
 * plot style for drawing rectangle round unselected nodes
 */
static plot_style_t pstyle_rect = {
	.stroke_type = PLOT_OP_TYPE_SOLID,
	.stroke_width = plot_style_int_to_fixed(1),
};


/**
 * plot style for drawing rectangle round selected nodes
 */
static plot_style_t pstyle_rect_sel = {
	.stroke_type = PLOT_OP_TYPE_SOLID,
	.stroke_width = plot_style_int_to_fixed(3),
};


/**
 * plot style for drawing rectangle round the cursor node
 */
static plot_style_t pstyle_rect_cursor = {
	.stroke_type = PLOT_OP_TYPE_DASH,
	.stroke_width = plot_style_int_to_fixed(3),
};


/**
 * plot style for font on unselected nodes
 */
static plot_font_style_t pfstyle_node = {
	.family = PLOT_FONT_FAMILY_SANS_SERIF,
	.size = 8 * PLOT_STYLE_SCALE,
	.weight = 400,
	.flags = FONTF_NONE,
};


/**
 * plot style for font on unselected nodes
 */
static plot_font_style_t pfstyle_node_sel = {
	.family = PLOT_FONT_FAMILY_SANS_SERIF,
	.size = 8 * PLOT_STYLE_SCALE,
	.weight = 900,
	.flags = FONTF_NONE,
};


/**
 * Recursively redraw a history entry.
 *
 * \param history history containing the entry
 * \param entry   entry to render
 * \param clip    redraw area
 * \param x       window x offset
 * \param y       window y offset
 * \param ctx     current redraw context
 */
static nserror
redraw_entry(struct history *history,
	     struct history_entry *entry,
	     struct history_entry *cursor,
	     struct rect *clip,
	     int x, int y,
	     const struct redraw_context *ctx)
{
	size_t char_offset;
	int actual_x;
	struct history_entry *child;
	int tailsize = 5;

	plot_style_t *pstyle;
	plot_font_style_t *pfstyle;
	struct rect rect;
	nserror res;

	/* setup plot styles */
	if (entry == history->current) {
		pstyle = &pstyle_rect_sel;
		pfstyle = &pfstyle_node_sel;
	} else {
		pstyle = &pstyle_rect;
		pfstyle = &pfstyle_node;
	}

	/* Only attempt to plot bitmap if it is present */
	if (entry->page.bitmap != NULL) {
		res = ctx->plot->bitmap(ctx,
					entry->page.bitmap,
					entry->x + x,
					entry->y + y,
					LOCAL_HISTORY_WIDTH,
					LOCAL_HISTORY_HEIGHT,
					0xffffff,
					0);
		if (res != NSERROR_OK) {
			return res;
		}
	}

	rect.x0 = entry->x - 1 + x;
	rect.y0 = entry->y - 1 + y;
	rect.x1 = entry->x + x + LOCAL_HISTORY_WIDTH;
	rect.y1 = entry->y + y + LOCAL_HISTORY_HEIGHT;
	res = ctx->plot->rectangle(ctx, pstyle, &rect);
	if (res != NSERROR_OK) {
		return res;
	}

	/* If this is the cursor, show that */
	if (entry == cursor) {
		rect.x0 -= 1;
		rect.y0 -= 1;
		rect.x1 += 2;
		rect.y1 += 2;
		ctx->plot->rectangle(ctx, &pstyle_rect_cursor, &rect);
	}

	res = guit->layout->position(plot_style_font, entry->page.title,
				     strlen(entry->page.title), LOCAL_HISTORY_WIDTH,
				     &char_offset, &actual_x);
	if (res != NSERROR_OK) {
		return res;
	}

	res = ctx->plot->text(ctx,
			      pfstyle,
			      entry->x + x,
			      entry->y + LOCAL_HISTORY_HEIGHT + 12 + y,
			      entry->page.title,
			      char_offset);
	if (res != NSERROR_OK) {
		return res;
	}

	/* for each child node draw a line and recurse redraw into it */
	for (child = entry->forward; child; child = child->next) {
		rect.x0 = entry->x + LOCAL_HISTORY_WIDTH + x;
		rect.y0 = entry->y + LOCAL_HISTORY_HEIGHT / 2 + y;
		rect.x1 = entry->x + LOCAL_HISTORY_WIDTH + tailsize + x;
		rect.y1 = entry->y + LOCAL_HISTORY_HEIGHT / 2 + y;
		res = ctx->plot->line(ctx, &pstyle_line, &rect);
		if (res != NSERROR_OK) {
			return res;
		}

		rect.x0 = entry->x + LOCAL_HISTORY_WIDTH + tailsize + x;
		rect.y0 = entry->y + LOCAL_HISTORY_HEIGHT / 2 + y;
		rect.x1 = child->x - tailsize + x;
		rect.y1 = child->y + LOCAL_HISTORY_HEIGHT / 2 + y;
		res = ctx->plot->line(ctx, &pstyle_line, &rect);
		if (res != NSERROR_OK) {
			return res;
		}

		rect.x0 = child->x - tailsize + x;
		rect.y0 = child->y + LOCAL_HISTORY_HEIGHT / 2 + y;
		rect.x1 = child->x + x;
		rect.y1 = child->y + LOCAL_HISTORY_HEIGHT / 2 + y;
		res = ctx->plot->line(ctx, &pstyle_line, &rect);
		if (res != NSERROR_OK) {
			return res;
		}

		res = redraw_entry(history, child, cursor, clip, x, y, ctx);
		if (res != NSERROR_OK) {
			return res;
		}
	}

	return NSERROR_OK;
}


/**
 * Find the history entry at a position.
 *
 * \param entry entry to search from
 * \param x coordinate
 * \param y coordinate
 * \return an entry if found, 0 if none
 */
static struct history_entry *
find_entry_position(struct history_entry *entry, int x, int y)
{
	struct history_entry *child;
	struct history_entry *found;

	if (!entry) {
		return NULL;
	}

	if ((entry->x <= x) &&
	    (x <= entry->x + LOCAL_HISTORY_WIDTH) &&
	    (entry->y <= y) &&
	    (y <= entry->y + LOCAL_HISTORY_HEIGHT)) {
		return entry;
	}

	for (child = entry->forward; child; child = child->next) {
		found = find_entry_position(child, x, y);
		if (found) {
			return found;
		}
	}

	return NULL;
}

/* exported interface documented in desktop/local_history.h */
nserror
local_history_scroll_to_cursor(struct local_history_session *session)
{
	rect cursor;

	if (session->cursor == NULL) {
		return NSERROR_OK;
	}

	cursor.x0 = session->cursor->x - LOCAL_HISTORY_RIGHT_MARGIN / 2;
	cursor.y0 = session->cursor->y - LOCAL_HISTORY_BOTTOM_MARGIN / 2;
	cursor.x1 = cursor.x0 + LOCAL_HISTORY_WIDTH +
			LOCAL_HISTORY_RIGHT_MARGIN / 2;
	cursor.y1 = cursor.y0 + LOCAL_HISTORY_HEIGHT +
			LOCAL_HISTORY_BOTTOM_MARGIN / 2;

	session->cw_t->scroll_visible(session->core_window_handle, &cursor);

	return NSERROR_OK;
}

/* exported interface documented in desktop/local_history.h */
nserror
local_history_init(struct core_window_callback_table *cw_t,
		   void *core_window_handle,
		   struct browser_window *bw,
		   struct local_history_session **session)
{
	nserror res;
	struct local_history_session *nses;

	res = ns_system_colour_char("Window", &pstyle_bg.fill_colour);
	if (res != NSERROR_OK) {
		return res;
	}
	pfstyle_node.background = pstyle_bg.fill_colour;
	pfstyle_node_sel.background = pstyle_bg.fill_colour;

	res = ns_system_colour_char("GrayText", &pstyle_line.stroke_colour);
	if (res != NSERROR_OK) {
		return res;
	}
	pstyle_rect.stroke_colour = pstyle_line.stroke_colour;
	pfstyle_node.foreground = pstyle_line.stroke_colour;

	res = ns_system_colour_char("ButtonText", &pstyle_rect_sel.stroke_colour);
	if (res != NSERROR_OK) {
		return res;
	}
	pfstyle_node_sel.foreground = pstyle_rect_sel.stroke_colour;

	res = ns_system_colour_char("Highlight", &pstyle_rect_cursor.stroke_colour);
	if (res != NSERROR_OK) {
		return res;
	}

	nses = calloc(1, sizeof(struct local_history_session));
	if (nses == NULL) {
		return NSERROR_NOMEM;
	}

	nses->cw_t = cw_t;
	nses->core_window_handle = core_window_handle;

	local_history_set(nses, bw);

	*session = nses;

	return NSERROR_OK;
}

/* exported interface documented in desktop/local_history.h */
nserror local_history_fini(struct local_history_session *session)
{
	free(session);

	return NSERROR_OK;
}


/* exported interface documented in desktop/local_history.h */
nserror
local_history_redraw(struct local_history_session *session,
		     int x,
		     int y,
		     struct rect *clip,
		     const struct redraw_context *ctx)
{
	struct rect r = {
		.x0 = clip->x0 + x,
		.y0 = clip->y0 + y,
		.x1 = clip->x1 + x,
		.y1 = clip->y1 + y,
	};

	if (session->bw == NULL) {
		return NSERROR_OK;
	}

	if (session->bw->history->start == NULL) {
		return NSERROR_OK;
	}

	ctx->plot->clip(ctx, &r);
	ctx->plot->rectangle(ctx, &pstyle_bg, &r);

	return redraw_entry(
		session->bw->history,
		session->bw->history->start,
		session->cursor,
		clip,
		x, y,
		ctx);
}

/* exported interface documented in desktop/local_history.h */
nserror
local_history_mouse_action(struct local_history_session *session,
			   enum browser_mouse_state mouse,
			   int x,
			   int y)
{
	struct history_entry *entry;
	bool new_window;

	if (session->bw == NULL) {
		return NSERROR_BAD_PARAMETER;
	}

	if ((mouse & (BROWSER_MOUSE_PRESS_1 | BROWSER_MOUSE_PRESS_2)) == 0) {
		return NSERROR_NOT_IMPLEMENTED;
	}

	entry = find_entry_position(session->bw->history->start, x, y);
	if (entry == NULL) {
		return NSERROR_NOT_FOUND;
	}

	if (entry == session->bw->history->current) {
		return NSERROR_PERMISSION;
	}

	if (mouse & BROWSER_MOUSE_PRESS_1) {
		new_window = false;
	} else if (mouse & BROWSER_MOUSE_PRESS_2) {
		new_window = true;
	} else {
		new_window = false;
	}

	browser_window_history_go(session->bw, entry, new_window);

	return NSERROR_OK;
}

/* exported interface documented in desktop/local_history.h */
bool
local_history_keypress(struct local_history_session *session, uint32_t key)
{
	switch (key) {
	case NS_KEY_NL:
	case NS_KEY_CR:
		/* pressed enter */
		if (session->cursor != session->bw->history->current) {
			browser_window_history_go(session->bw, session->cursor,
						  false);
			local_history_scroll_to_cursor(session);
			session->cw_t->invalidate(session->core_window_handle, NULL);
		}
		/* We have handled this keypress */
		return true;
	case NS_KEY_LEFT:
		/* Go to parent */
		if (session->cursor->back) {
			session->cursor = session->cursor->back;
			local_history_scroll_to_cursor(session);
			session->cw_t->invalidate(session->core_window_handle, NULL);
		}
		/* We have handled this keypress */
		return true;
	case NS_KEY_RIGHT:
		/* Go to preferred child if there is one */
		if (session->cursor->forward_pref) {
			session->cursor = session->cursor->forward_pref;
			local_history_scroll_to_cursor(session);
			session->cw_t->invalidate(session->core_window_handle, NULL);
		}
		/* We have handled this keypress */
		return true;
	case NS_KEY_DOWN:
		/* Go to next sibling down, if there is one */
		if (session->cursor->next) {
			session->cursor = session->cursor->next;
			local_history_scroll_to_cursor(session);
			session->cw_t->invalidate(session->core_window_handle, NULL);
		}
		/* We have handled this keypress */
		return true;
	case NS_KEY_UP:
		/* Go to next sibling up, if there is one */
		if (session->cursor->back) {
			struct history_entry *ent = session->cursor->back->forward;
			while (ent->next != NULL && ent->next != session->cursor) {
				ent = ent->next;
			}
			session->cursor = ent;
			local_history_scroll_to_cursor(session);
			session->cw_t->invalidate(session->core_window_handle, NULL);
		}
		/* We have handled this keypress */
		return true;
	}
	return false;
}

/* exported interface documented in desktop/local_history.h */
nserror
local_history_set(struct local_history_session *session,
		  struct browser_window *bw)
{
	session->bw = bw;
	session->cursor = NULL;

	if (bw != NULL) {
		assert(session->bw->history != NULL);
		session->cursor = bw->history->current;

		session->cw_t->update_size(session->core_window_handle,
					   session->bw->history->width,
					   session->bw->history->height);
		local_history_scroll_to_cursor(session);
	}

	return NSERROR_OK;
}


/* exported interface documented in desktop/local_history.h */
nserror
local_history_get_size(struct local_history_session *session,
		       int *width,
		       int *height)
{
	*width = session->bw->history->width + 20;
	*height = session->bw->history->height + 20;

	return NSERROR_OK;
}


/* exported interface documented in desktop/local_history.h */
nserror
local_history_get_url(struct local_history_session *session,
		      int x, int y,
		      nsurl **url_out)
{
	struct history_entry *entry;

	if (session->bw == NULL) {
		return NSERROR_BAD_PARAMETER;
	}

	entry = find_entry_position(session->bw->history->start, x, y);
	if (entry == NULL) {
		return NSERROR_NOT_FOUND;
	}

	*url_out = nsurl_ref(entry->page.url);

	return NSERROR_OK;
}
