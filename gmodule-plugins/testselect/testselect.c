#include <stdio.h>
#include "client.h"

int main (int argc, char *argv[])
{
	client_info info;
	selection_range range;
	int docid, contextid, pos;
	char *text;

	info.menu_location = "[Plugins]Test Selection";

	contextid = client_init (&argc, &argv, &info);
	docid = client_document_current (contextid);

	text = client_text_get_selection_text (docid);
	range = client_document_get_selection_range (docid);
	pos = client_document_get_position (docid);

	if (text == NULL)
		printf ("No text selected.\n");
	else
		printf ("\"%s\" selected\n", text);
	printf ("Cursor is at position %i\n", pos);
	printf ("Selection starts at point %i, and ends at point %i\n", start, end);

	exit (0);
}

