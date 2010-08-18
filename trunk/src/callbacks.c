#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "netdisc.h"
#include <string.h>
#include <stdlib.h>

device *niclist=NULL;
netnode *nodelist=NULL;

static void setup_tree_view_eth (GtkWidget *treeview);
static void setup_tree_view_nodes (GtkWidget *treeview);
static void setup_tree_view_proto (GtkWidget *treeview);

int selected_iface=0;

void free_nic_list(device *list)
{
    if (list!=NULL)
    {
        device *temp;
        temp=list;
        list=list->next;
        free(temp);
    }
}
void free_node_list(netnode *list)
{
    if (list!=NULL)
    {
        netnode *temp;
        temp=list;
        list=list->next;
        free(temp);
    }
}

gboolean
on_rmanage_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{

    return FALSE;
}


gboolean
on_rmanage_destroy_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{

    return FALSE;
}


void
on_rmanage_destroy                     (GtkObject       *object,
                                        gpointer         user_data)
{
    gtk_main_quit();

}


void
on_new1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_open1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_save1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_save_as1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    free_nic_list(niclist);
    free_node_list(nodelist);
    gtk_main_quit();
}


void
on_cut1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_copy1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_paste1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_delete1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dialog1;
    dialog1=create_About();
    gtk_widget_show (dialog1);
}


void
on_nictree_row_activated               (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkListStore *store;
    extern GtkWidget *nodetree;
    GtkWidget *netwin;
    //extern GtkWidget *status;
    device *temp=NULL;
    netnode *temp_node=NULL;
    //node_tree=(GtkTreeView *)user_data;
    temp=niclist;
    g_print("row activated signal\n");
// on select interface do save the interface info for scan to happen
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
    if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection),&model, &iter))
    {
        gtk_tree_model_get (model, &iter, 0, &selected_iface, -1);
        g_print("Selected interface number : = %d \n", selected_iface);
    }
    if (selected_iface>0)
    {
        while (temp)
        {
            if (temp->if_number==selected_iface)
            {
                strncpy(current_nic_dev,temp->name,15);
                break;
            }
            else
                temp=temp->next;
        }
        netwin=create_netwindow();
        gtk_widget_show(netwin);
        g_print("Please Wait searching your network.........\n");
        if (nodelist!=NULL)
            free_node_list(nodelist);
        nodelist=make_nodes_list(temp);
        temp_node=nodelist;
        setup_tree_view_nodes((GtkWidget *)nodetree);
        store = gtk_list_store_new (4,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_BOOLEAN);
        while (temp_node)
        {
            //if(temp_node->alive==1)
            {
                gtk_list_store_append (store, &iter);
                gtk_list_store_set (store, &iter, 0, temp_node->ip, 1,temp_node->mac,2,temp_node->hostname,3, temp_node->alive==1?1:0, -1);
            }
            temp_node=temp_node->next;
        }
        gtk_tree_view_set_model (GTK_TREE_VIEW (nodetree), GTK_TREE_MODEL (store));
        g_object_unref(store);
    }

}


void
on_detect_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkTreeView *if_treeview;
    GtkListStore *store;
    static GtkTreeIter iter;
    device *temp;
    if_treeview=(GtkTreeView *)user_data;
    if (niclist==NULL)
        niclist = make_eth_dev_list();
    if (niclist)
    {
        temp=niclist;
        setup_tree_view_eth((GtkWidget *)if_treeview);
        store=store = gtk_list_store_new (7,G_TYPE_INT,G_TYPE_BOOLEAN,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
        while (temp)
        {
            gtk_list_store_append (store, &iter);
            gtk_list_store_set (store, &iter, 0, temp->if_number, 1, (temp->configured==1)?1:0,2,temp->name,3,temp->ip,4,temp->mask,5,temp->broadcast,6,temp->mac, -1);
            temp=temp->next;
        }
        gtk_tree_view_set_model (GTK_TREE_VIEW (if_treeview), GTK_TREE_MODEL (store));
        g_object_unref(store);
    }
}


void
on_about_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *dialog1;
    dialog1=create_About();
    gtk_widget_show (dialog1);
}


void
on_quit_clicked                        (GtkButton       *button,
                                        gpointer         user_data)
{
    free_nic_list(niclist);
    free_node_list(nodelist);
    gtk_main_quit();
}


void
on_about_ok_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    gtk_widget_hide((GtkWidget *)user_data);
}


void
on_netwindow_destroy                   (GtkObject       *object,
                                        gpointer         user_data)
{
    gtk_main_quit();
}


gboolean
on_netwindow_delete_event              (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{

    return FALSE;
}


void
on_nodetree_row_activated              (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{
    GtkWidget *command;
    extern GtkWidget *prototree;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter,proto_iter;
    GtkListStore *store;
    int listen=0;
    gchar *ip=NULL,*host_name=NULL,buff[1024];
    int compat;
    application *app_list=NULL,*temp=NULL;
    //selection
    g_print("row activated signal\n");
    // on select interface do save the interface info for scan to happen
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
    if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection),&model, &iter))
    {
        g_print("selected some row\n");
        //gtk_tree_model_get (model, &iter, 3, &listen, -1);
        //if(listen>0)
        {
            gtk_tree_model_get(model, &iter, 0, &ip, -1);
            g_print("Selected machine ip : = %s \n",ip);
            gtk_tree_model_get(model, &iter, 2, &host_name, -1);
            g_print("Selected machine host_name : = %s \n",host_name);
            sprintf(buff,"%s(%s)",host_name,ip);
            command = create_Command ();
            gtk_window_set_title(GTK_WINDOW(command),buff);
            setup_tree_view_proto(prototree);
            store = gtk_list_store_new (4,G_TYPE_STRING,G_TYPE_INT,G_TYPE_STRING,G_TYPE_BOOLEAN);
            app_list=get_configured_application_list(CONF_FILE);
            temp=app_list;
            while (temp)
            {
                //g_print("nice we have an app-list\n");
                gtk_list_store_append (store, &proto_iter);
                gtk_list_store_set (store, &proto_iter, 0, temp->protocol, 1,temp->port,2,temp->app,3, (is_port_open(ip,temp->port)==1)?1:0, -1);
                temp=temp->next;
            }
            gtk_tree_view_set_model (GTK_TREE_VIEW (prototree), GTK_TREE_MODEL (store));
            g_object_unref(store);
            gtk_widget_show (command);
        }
    }
}


void
on_prototree_row_activated             (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{
    //when we double click on a row in command window
    GtkWidget *usrentry=NULL,*passwd=NULL;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    int ibeg=0,iend=0;
    char *appname=NULL,*usrname=NULL,buffer[1024],*title=NULL,ip[16]="";
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
    if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection),&model, &iter))
    {
        gtk_tree_model_get (model, &iter, 2, &appname, -1);

        g_print("Selected application = %s \n", appname);
    }
    usrentry=lookup_widget((GtkWidget *)user_data,"username_entry");
    title=gtk_window_get_title(GTK_WINDOW(user_data));
    printf("title -> %s\n",title);
    ibeg=strchr(title,'(');
    iend=strchr(title,')');
    printf("ip -> %s ( at %d and ) at %d\n",ibeg,ibeg,iend);
    strncat(ip,ibeg+1,iend-ibeg-1);
    printf("%s= ip\n",ip);
    //passwd=lookup_widget((GtkWidget *)user_data,"passwd_entry");
    if (usrentry)
        usrname=gtk_entry_get_text((GtkEntry *)usrentry);
    if (strlen(usrname))
    {
        if (strcmp(appname,"telnet")==0)
            sprintf(buffer,"xterm -e %s %s -l%s",appname,ip,usrname);
        if (strcmp(appname,"ssh")==0)
            sprintf(buffer,"xterm -e %s %s@%s",appname,usrname,ip);
    }
    else
        sprintf(buffer,"xterm -e %s %s",appname,ip);
    printf("fired command is %s\n",buffer);
    system(buffer);
}
static void
setup_tree_view_eth (GtkWidget *treeview)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    static int setup;

    /* Create a new GtkCellRendererText, add it to the tree view column and
     * append the column to the tree view. */
    if(setup==0)
    {
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes
                 ("SL#", renderer, "text", 0, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes
                 ("Configured", renderer, "text", 1, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes
                 ("Name", renderer, "text", 2, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes
                 ("IP", renderer, "text", 3, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes
                 ("Mask", renderer, "text", 4, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes
                 ("Broadcast", renderer, "text", 5, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes
                 ("MAC", renderer, "text", 6, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
    }
    setup++;
}
static void
setup_tree_view_nodes (GtkWidget *treeview)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    static int setup;
    /* Create a new GtkCellRendererText, add it to the tree view column and
     * append the column to the tree view. */
    //if(setup==0)
    {
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes
                 ("IP", renderer, "text", 0, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes
                 ("MAC", renderer, "text", 1, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes
                 ("HostName", renderer, "text", 2, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes
                 ("Alive", renderer, "text", 3, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    }
    setup++;

}
static void
setup_tree_view_proto (GtkWidget *treeview)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    static int setup;
    /* Create a new GtkCellRendererText, add it to the tree view column and
     * append the column to the tree view. */
    //if(setup==0)
    {
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes
                 ("Protocol", renderer, "text", 0, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes
                 ("port", renderer, "text", 1, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes
                 ("Application", renderer, "text", 2, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes
                 ("Open", renderer, "text", 3, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    }
    setup++;

}
