#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <bonobo/bonobo-generic-factory.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-context.h>
#include <bonobo-activation/bonobo-activation-register.h>


#include "GNOME_Stardict.h"
#include "stardict.h"

#include "stardict-application-server.h"


static void stardict_application_server_class_init (StardictApplicationServerClass *klass);
static void stardict_application_server_init (StardictApplicationServer *a);
static void stardict_application_server_object_finalize (GObject *object);
static GObjectClass *stardict_application_server_parent_class;

static BonoboObject *
stardict_application_server_factory (BonoboGenericFactory *this_factory,
			   const char *iid,
			   gpointer user_data)
{
        StardictApplicationServer *a;
        
        a  = (StardictApplicationServer *)g_object_new (STARDICT_APPLICATION_SERVER_TYPE, NULL);

        return BONOBO_OBJECT (a);
}

BonoboObject *
stardict_application_server_new (GdkScreen *screen)
{
	BonoboGenericFactory *factory;
	char                 *display_name;
	char                 *registration_id;

	/* We must ensure an instance of stardict per screen
	 * as stardict has no multiscreen support 
	 */
	display_name = gdk_screen_make_display_name (screen);
	registration_id = bonobo_activation_make_registration_id (
					"OAFIID:GNOME_Stardict_Factory",
					display_name);

	factory = bonobo_generic_factory_new (registration_id,
					      stardict_application_server_factory,
					      NULL);

	g_free (display_name);
	g_free (registration_id);

	return BONOBO_OBJECT (factory);
}

static void
impl_stardict_application_server_queryWord (PortableServer_Servant _servant,
				   const CORBA_char *str,
				   CORBA_Environment * ev)
{
	gpAppFrame->Query((const gchar *)str);
}

static void
impl_stardict_application_server_grabFocus (PortableServer_Servant _servant,
					 CORBA_Environment * ev)
{
	gtk_window_present (GTK_WINDOW (gpAppFrame->window));
}

static void
impl_stardict_application_server_hide (PortableServer_Servant _servant,
					 CORBA_Environment * ev)
{
	gtk_widget_hide(gpAppFrame->window);
}

static void
impl_stardict_application_server_quit (PortableServer_Servant _servant,
                                CORBA_Environment * ev)
{
	gpAppFrame->Quit();
}

static void
stardict_application_server_class_init (StardictApplicationServerClass *klass)
{
        GObjectClass *object_class = (GObjectClass *) klass;
        POA_GNOME_Stardict_Application__epv *epv = &klass->epv;

        stardict_application_server_parent_class = (GObjectClass *)g_type_class_peek_parent (klass);

        object_class->finalize = stardict_application_server_object_finalize;

        /* connect implementation callbacks */
	epv->queryWord              = impl_stardict_application_server_queryWord;
	epv->grabFocus              = impl_stardict_application_server_grabFocus;
	epv->hide              = impl_stardict_application_server_hide;	
	epv->quit              = impl_stardict_application_server_quit;
}

static void
stardict_application_server_init (StardictApplicationServer *c) 
{
}

static void
stardict_application_server_object_finalize (GObject *object)
{
        StardictApplicationServer *a = STARDICT_APPLICATION_SERVER (object);

        stardict_application_server_parent_class->finalize (G_OBJECT (a));
}

BONOBO_TYPE_FUNC_FULL (
        StardictApplicationServer,                    
        GNOME_Stardict_Application, 
        BONOBO_TYPE_OBJECT,           
        stardict_application_server);
