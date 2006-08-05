#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <string>
#include <sstream>
#include <memory>
#include <iostream>

#include "docklet.h"

class Label {
public:
	Label(GtkWidget *parent);
	void set_text(const std::string&);
	std::string get_text() const;
private:
	GtkLabel *g_label_;
};

Label::Label(GtkWidget *parent)
{
	g_label_ = GTK_LABEL(gtk_label_new("Label"));
	gtk_container_add(GTK_CONTAINER(parent), GTK_WIDGET(g_label_));
}

void Label::set_text(const std::string& text)
{
	gtk_label_set_text(g_label_, text.c_str());
}

std::string Label::get_text() const 
{
	return gtk_label_get_text(g_label_);
}

class Test {
public:
	Test(int argc, char *argv[]);
	int run();
private:
	GThread *thid_;
	std::auto_ptr<Label> label_;

	static void destroy(GtkWidget *widget, gpointer data);
	static void *thread_helper(void *args);
};

void Test::destroy(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
}

int Test::run() 
{
	thid_ = g_thread_create(thread_helper, this, FALSE, NULL);
	gtk_main();
	gdk_threads_leave();
		
	return EXIT_SUCCESS;
}

Test::Test(int argc, char *argv[]) : thid_(NULL)
{
  g_thread_init(NULL);
  gdk_threads_init();
  gdk_threads_enter();

  gtk_init(&argc, &argv);

  srand((unsigned int)time(NULL));

  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_signal_connect(GTK_OBJECT(window), "destroy",
		     GTK_SIGNAL_FUNC(destroy), NULL);

  gtk_container_set_border_width(GTK_CONTAINER(window), 10);

  label_.reset(new Label(window));
  
  gtk_widget_show_all(window);
}

void *Test::thread_helper(void *args)
{
	Test *t = static_cast<Test *>(args);

	for (int i = 0; i < 100; ++i) {
		/* sleep a while */
		usleep(100);
		std::ostringstream os;
		os << i;
		/* get GTK thread lock */
		gdk_threads_enter();
		
		/* set label text */      
		t->label_->set_text(os.str());
		/* release GTK thread lock */
		gdk_threads_leave();

		usleep(100);
		gdk_threads_enter();
		if (t->label_->get_text() != os.str())
			std::cerr << "test failed" << std::endl;
		gdk_threads_leave();
	}

	destroy(NULL, NULL);
	return NULL;
}

int main (int argc, char *argv[])
{  
	Test test(argc, argv);
	
	return test.run();
}
