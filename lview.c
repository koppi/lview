/* L-System viewer by Jakob Flierl, based on Motif code by Andreas Zeller */

#include <stdio.h>
#include <math.h>

#include <gtk/gtk.h>

#define XSIZE 480
#define YSIZE 480

static GtkWidget *drawing_area, *prev, *next, *statusbar;
static gint graphics_index = 0;
static gint context_id;
static char VERSION[] = "0.0.3";

static char *read_graphic()
{
    int buffer_size = BUFSIZ;
    char *buffer = g_malloc(buffer_size * sizeof(char));
    int number, len, c;

    scanf("%d", &number); /* ignored */

    len = 0;
    while ((c = getchar()) != EOF && c != '\n')
    {
	if (len >= buffer_size)
	    buffer = g_realloc(buffer, buffer_size += BUFSIZ);
	buffer[len++] = c;
    }

    buffer[len] = '\0';
    return buffer;
}

static char **read_graphics()
{
    int graphics_size = 32;
    char **graphics = (char **)g_malloc(graphics_size * sizeof(char *));
    int len, c;

    len = 0;
    while ((c = getchar()) != EOF)
    {
	if (len >= graphics_size)
	    graphics = 
		(char **)g_realloc((char *)graphics, graphics_size += 32);
	ungetc(c, stdin);
	graphics[len++] = read_graphic();
    }

    graphics[len] = NULL;
    return graphics;
}

static void turtle(GtkWidget *widget, char **command, 
		   gdouble scale_x, gdouble scale_y,
		   gdouble offset_x, gdouble offset_y,
		   gdouble *min_x, gdouble *min_y,
		   gdouble *max_x, gdouble *max_y)
{
    static gdouble x = 0.0;
    static gdouble y = 0.0;
    static gdouble angle = 0.0;
    static gdouble h = 1.0;
    static gdouble delta = M_PI / 12;

    gdouble x0 = x;
    gdouble y0 = y;
    gdouble angle0 = angle;

#ifdef DEBUG
    printf("%f %f %f %f\n", *min_x, *min_y, *max_x, *max_y);
#endif

    while (**command != '\0')
    {
	switch (*(*command)++)
	{
	case '@':			/* initialize */
	    x0 = x = 0.0;
	    y0 = y = 0.0;
	    angle0 = angle = 0.0;
	    break;
		
	case 'F': // Move forward, drawing a line.
	    x += h * cos(angle);
	    y += h * sin(angle);
	    if (scale_x > 0.0 || scale_y > 0.0)
	    {
	      gdk_draw_line(widget->window,
			    widget->style->fg_gc[widget->state],
			    offset_x + (x0 - *min_x) * scale_x, 
			    offset_y + (y0 - *min_y) * scale_y, 
			    offset_x + (x - *min_x) * scale_x, 
			    offset_y + (y - *min_y) * scale_y);
#ifdef DEBUG
              printf("Draw %f %f %f %f\n", x0, y0, x1, y1);
#endif
	    }
	    x0 = x;
	    y0 = y;
	    break;
		
	case 'f': // Move forward, without drawing line
	    x += h * cos(angle);
	    y += h * sin(angle);
	    x0 = x;
	    y0 = y;
	    break;

	case '+': // Rotate by delta degrees in the positive direction
	    angle += delta;
	    break;

	case '-': // Rotate by delta degrees in the negative direction
	    angle -= delta;
	    break;

	case '[': // Push the draw state (position and angle) on the stack
	case '(':
	    x0 = x;
	    y0 = y;
	    angle0 = angle;

	    turtle(widget, command,
		   scale_x, scale_y,
		   offset_x, offset_y,
		   min_x, min_y, max_x, max_y);

	    x = x0;
	    y = y0;
	    angle = angle0;
	    break;

	case ']': // Pop the draw state (position and angle) off the stack
	case ')':
	case '\0':
	    return;
	}

	if (scale_x <= 0.0 || scale_y <= 0.0)
	{
#ifdef DEBUG
	    printf("Now at %f %f\n", x, y);
#endif
	    if (x < *min_x)
		*min_x = x;
	    if (y < *min_y)
		*min_y = y;
	    if (x > *max_x)
		*max_x = x;
	    if (y > *max_y)
		*max_y = y;
	}
    }
}

static void turtles(GtkWidget *widget, char *command,
		    gdouble scale_x, gdouble scale_y,
		    gdouble offset_x, gdouble offset_y,
		    gdouble *min_x, gdouble *min_y,
		    gdouble *max_x, gdouble *max_y)
{
    char *cmd = "@";
    turtle(widget, &cmd,
	   scale_x, scale_y,
	   offset_x, offset_y,
	   min_x, min_y,
	   max_x, max_y);

    cmd = command;

    turtle(widget, &cmd,
	   scale_x, scale_y,
	   offset_x, offset_y,
	   min_x, min_y,
	   max_x, max_y);
}

static void redraw(GtkWidget *widget, char **graphics)
{
    char *command = graphics[graphics_index];

    gdouble min_x = 0.0;
    gdouble min_y = 0.0;

    gdouble max_x = 0.0;
    gdouble max_y = 0.0;

    gdouble scale_x, scale_y;
    gdouble offset_x = 0.0;
    gdouble offset_y = 0.0;
    gdouble width, height;

    if (command == NULL)
	return;

    gdk_draw_rectangle (widget->window, widget->style->bg_gc[widget->state],
			TRUE, 0, 0,
			widget->allocation.width,
			widget->allocation.height);

    turtles(widget, command, 0.0, 0.0, 0.0, 0.0,
	    &min_x, &min_y, &max_x, &max_y);

    width = widget->allocation.width;
    height = widget->allocation.height;

    offset_x = 0.0;
    offset_y = 0.0;
    scale_x = width;
    scale_y = height;

    if (max_x - min_x > 0.0)
	scale_x = width / (max_x - min_x);
    else
	offset_x = width / 2;

    if (max_y - min_y > 0.0)
	scale_y = height / (max_y - min_y);
    else
	offset_y = height / 2;

#ifdef DEBUG
    printf("min %f %f\nmax %f %f\nscale %f %f\noffset %f %f\n", 
	   min_x, min_y, max_x, max_y, 
	   scale_x, scale_y, offset_x, offset_y);
#endif

    turtles(widget, command,
	    scale_x, scale_y, 
	    offset_x, offset_y,
	    &min_x, &min_y, 
	    &max_x, &max_y);
}

static void update_sensitivity(char **graphics)
{
  gtk_widget_set_sensitive(prev, graphics_index > 0);
  gtk_widget_set_sensitive(next, graphics[graphics_index + 2] != NULL);
}

void set_statusbar()
{
  char buff[100];

  gtk_statusbar_pop(GTK_STATUSBAR(statusbar), GPOINTER_TO_INT(context_id));
  g_snprintf(buff,100, "LView %s - %d iterations.", VERSION, graphics_index);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar),
		     GPOINTER_TO_INT(context_id), buff);
}

static gint expose_event(GtkWidget *widget,
			 GdkEventExpose *event,
			 gpointer data)
{
  char **graphics = (char **)data;

  set_statusbar();
  redraw(widget, graphics);
  update_sensitivity(graphics);

  return FALSE;
}

static gint PrevCB(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    char **graphics = (char **)data;

    if (graphics_index > 0)
	graphics_index--;

    gtk_signal_emit_by_name(GTK_OBJECT(drawing_area), "expose_event",
			    event, graphics);

    return TRUE;
}

static gint NextCB(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    char **graphics = (char **)data;

    if (graphics[graphics_index + 1] != NULL)
	graphics_index++;

    gtk_signal_emit_by_name(GTK_OBJECT(drawing_area), "expose_event",
			    event, graphics);

    return TRUE;
}

int main(int argc, char *argv[])
{
  GtkWidget *window, *vbox, *toolbar, *handlebox, *quit;
  char **graphics;

  gtk_init(&argc, &argv);

  graphics = read_graphics();

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "LView");
  gtk_signal_connect(GTK_OBJECT(window), "delete_event",
		     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
  gtk_signal_connect(GTK_OBJECT(window), "destroy",
		     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

  /* vertical layout box */
  vbox = gtk_vbox_new(0, FALSE);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  /* toolbar button box */
  handlebox = gtk_handle_box_new();
  toolbar = gtk_toolbar_new();

  gtk_container_add(GTK_CONTAINER(handlebox), toolbar);

  /* previous button */
  prev = gtk_button_new_with_label(" Previous ");
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), prev,
			    "Display less iterations.", "Private");

  gtk_signal_connect(GTK_OBJECT(prev), "clicked",
		     GTK_SIGNAL_FUNC(PrevCB), graphics);

  /* next button */
  next = gtk_button_new_with_label("     Next     ");
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), next,
			    "Display more iterations.", "Private");

  gtk_signal_connect(GTK_OBJECT(next), "clicked",
		     GTK_SIGNAL_FUNC(NextCB), graphics);

  /* quit button */
  quit = gtk_button_new_with_label("     Quit     ");
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), quit,
			    "Quit this program.", "Private");

  gtk_signal_connect(GTK_OBJECT(quit), "clicked",
		     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

  gtk_box_pack_start(GTK_BOX(vbox), handlebox, FALSE, FALSE, 0);

  /* the lsystem's drawing area */
  drawing_area = gtk_drawing_area_new();
  gtk_widget_set_usize(drawing_area, XSIZE, YSIZE);
  gtk_box_pack_start(GTK_BOX(vbox), drawing_area, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (drawing_area), "expose_event",
                      (GtkSignalFunc) expose_event, graphics);

  /* status bar */
  statusbar = gtk_statusbar_new();
  gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);
  context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "LView");

  update_sensitivity(graphics);

  gtk_widget_show_all(window);
  gtk_main();

  return 0;  /* never reached */
}
