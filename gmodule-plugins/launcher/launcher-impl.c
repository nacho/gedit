/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <launcher-impl.h>

/*** Implementation stub prototypes ***/
static void impl_gEdit_launcher__destroy (impl_POA_gEdit_launcher * servant,
					  CORBA_Environment * ev);

/*** epv structures ***/
static PortableServer_ServantBase__epv impl_gEdit_launcher_base_epv =
{
	NULL,						/* _private data */
	(gpointer) & impl_gEdit_launcher__destroy,	/* finalize routine */
	NULL,						/* default_POA routine */
};
static POA_gEdit_launcher__epv impl_gEdit_launcher_epv =
{
	NULL,						/* _private */
	(gpointer) & impl_gEdit_launcher_open_file,
};

/*** vepv structures ***/
static POA_gEdit_launcher__vepv impl_gEdit_launcher_vepv =
{
	&impl_gEdit_launcher_base_epv,
	&impl_gEdit_launcher_epv,
};

/*** Stub implementations ***/
gEdit_launcher 
impl_gEdit_launcher__create (PortableServer_POA poa, CORBA_long context,
			     CORBA_Environment * ev)
{
	gEdit_launcher retval;
	impl_POA_gEdit_launcher *newservant;
	PortableServer_ObjectId *objid;

	newservant = g_new0 (impl_POA_gEdit_launcher, 1);
	newservant->servant.vepv = &impl_gEdit_launcher_vepv;
	newservant->poa = poa;
	newservant->context = context;

	POA_gEdit_launcher__init ((PortableServer_Servant) newservant, ev);
	objid = PortableServer_POA_activate_object (poa, newservant, ev);
	CORBA_free (objid);
	retval = PortableServer_POA_servant_to_reference (poa, newservant, ev);

	return retval;
}

CORBA_long
impl_gEdit_launcher_open_file (impl_POA_gEdit_launcher * servant,
			       CORBA_char * filename, CORBA_Environment * ev)
{
	gint docid;

	g_return_if_fail (servant != NULL);
	g_return_if_fail (filename != NULL);

	docid = gE_plugin_document_open ((gint) servant->context, filename);
	g_return_if_fail (docid != 0);

	gE_plugin_document_show (docid);
	return (CORBA_long) docid;
}

/* You shouldn't call this routine directly without first
 * deactivating the servant... */
void
impl_gEdit_launcher__destroy (impl_POA_gEdit_launcher * servant,
			      CORBA_Environment * ev)
{
	POA_gEdit_launcher__fini ((PortableServer_Servant) servant, ev);
	g_free (servant);
}
