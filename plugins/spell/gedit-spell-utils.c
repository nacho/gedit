#include "gedit-spell-utils.h"
#include <gtksourceview/gtksourcebuffer.h>

gboolean
gedit_spell_utils_skip_no_spell_check (GtkTextIter *start,
                                       GtkTextIter *end)
{
	GtkSourceBuffer *buffer = GTK_SOURCE_BUFFER (gtk_text_iter_get_buffer (start));

	while (gtk_source_buffer_iter_has_context_class (buffer, start, "no-spell-check"))
	{
		GtkTextIter last = *start;

		if (!gtk_source_buffer_iter_forward_to_context_class_toggle (buffer, start, "no-spell-check"))
		{
			return FALSE;
		}

		if (gtk_text_iter_compare (start, &last) <= 0)
		{
			return FALSE;
		}

		gtk_text_iter_forward_word_end (start);
		gtk_text_iter_backward_word_start (start);

		if (gtk_text_iter_compare (start, &last) <= 0)
		{
			return FALSE;
		}

		if (gtk_text_iter_compare (start, end) >= 0)
		{
			return FALSE;
		}
	}

	return TRUE;
}

