// vim: noet ts=4 sw=4 sts=0
#include <Elementary.h>
#include <Imlib2.h>
#include "ui.h"
#include "util.h"
#include "ops.h"


// simpler callback add
#define $$$$($object, $event, $callback, $arg) \
	evas_object_smart_callback_add($object, $event, \
			(void *)($callback), (void *)($arg))
#define $$$($object, $event, $callback, $arg) \
	evas_object_event_callback_add($object, $event, \
			(void *)($callback), (void *)($arg))


typedef struct Operator
{
	const char * name;
	int nprop;
	bool (*poll)(float props[]);
	Evas_Object * table;
}
Operator;

static Operator ops[UI_MAX_OPERATORS];
static int ops_used;


static Evas_Object * win;
static Evas_Object * nodes;
static Evas_Object * props;
static Evas_Object * menu_node;
static Elm_Object_Item * menu_add;
static Elm_Object_Item * menu_delete;
static Evas_Object * props_current;

static EAPI_MAIN int elm_main(int argc, char * argv[]);


void ui_run()
{
	ELM_MAIN()
	main(0, NULL);
}

static EAPI_MAIN int elm_main(int argc, char * argv[])
{
	//------------------- main window
	win = elm_win_util_standard_add("ipu", "Image Proc Unit");
	evas_object_resize(win, 800, 600);
	$$$$(win, "delete,request", &elm_exit, NULL);

	//------------------- main vbox
	$_(box, elm_box_add(win));
	evas_object_size_hint_weight_set(box,
			EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, box);
	evas_object_show(box);

	//------------------- toolbar
	$_(toolbar, elm_toolbar_add(win));
	evas_object_size_hint_align_set(toolbar, EVAS_HINT_FILL, 0);
	elm_box_pack_end(box, toolbar);
	evas_object_show(toolbar);

	// toolbar items
	elm_toolbar_item_append(toolbar, "document-new", "New", NULL, NULL);
	elm_toolbar_item_append(toolbar, "document-open", "Open", NULL, NULL);
	elm_toolbar_item_append(toolbar, "document-save", "Save", NULL, NULL);
	elm_toolbar_item_append(toolbar, "system-run", "Settings", NULL, NULL);
	elm_toolbar_item_append(toolbar, "edit-delete", "Exit",
			(void *)&elm_exit, NULL);

	//------------------- content hbox
	$_(content, elm_box_add(win));
	elm_box_horizontal_set(content, EINA_TRUE);
	evas_object_size_hint_weight_set(content,
			EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(content,
			EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_pack_end(box, content);
	evas_object_show(content);

	//------------------- nodes
	// frame
	$_(nodes_frame, elm_frame_add(win));
	elm_object_text_set(nodes_frame, "Nodes");
	evas_object_size_hint_weight_set(nodes_frame, 0.5, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(nodes_frame,
			EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_pack_end(content, nodes_frame);
	evas_object_show(nodes_frame);

	// list
	nodes = elm_list_add(win);
	evas_object_size_hint_weight_set(nodes,
			EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(nodes, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_content_set(nodes_frame, nodes);
	evas_object_show(nodes);

	// menu
	menu_node = elm_menu_add(win);
	// show menu when right clicked
	$$$(nodes, EVAS_CALLBACK_MOUSE_DOWN,
			$(void, (void * $1, void * $2, void * $3,
					Evas_Event_Mouse_Down * ev) {
				if (ev->button == 3) {
					// when no item selected, disable "Delete" item
					elm_object_item_disabled_set(menu_delete,
							!elm_list_selected_item_get(nodes));

					elm_menu_move(menu_node, ev->canvas.x, ev->canvas.y);
					evas_object_show(menu_node);
				}
			}), NULL);

	menu_add = elm_menu_item_add(menu_node, NULL,
			"document-new", "Add", NULL, NULL);

	menu_delete = elm_menu_item_add(menu_node, NULL,
		"edit-delete", "Delete", (void *)$(void, () {
			$_(item, elm_list_selected_item_get(nodes));
			$_(o, elm_menu_item_object_get(item));
			Operator * op = evas_object_data_get(o, "ipu:operator");

			elm_object_content_unset(props);
			evas_object_hide(op->table);
			props_current = NULL;

			free(evas_object_data_get(o, "ipu:props"));
			elm_object_item_del(item);
		}), NULL);

	//------------------- properties
	// frame
	$_(props_frame, elm_frame_add(win));
	elm_object_text_set(props_frame, "Properties");
	evas_object_size_hint_weight_set(props_frame,
			EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(props_frame,
			EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_pack_end(content, props_frame);
	evas_object_show(props_frame);

	// scroller
	props = elm_scroller_add(win);
	evas_object_size_hint_weight_set(props,
			EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(props, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_content_set(props_frame, props);
	evas_object_show(props);

	//------------------- image stack
	// frame
	$_(stack_frame, elm_frame_add(win));
	elm_object_text_set(stack_frame, "Image Stack");
	evas_object_size_hint_weight_set(stack_frame, 0.5, 1);
	evas_object_size_hint_align_set(stack_frame,
			EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_pack_end(content, stack_frame);
	evas_object_show(stack_frame);

	// list
	$_(stack, elm_list_add(win));
	evas_object_size_hint_weight_set(stack,
			EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(stack, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_content_set(stack_frame, stack);
	evas_object_show(stack);

	// list items
	elm_list_item_append(stack, "Hi!", NULL, NULL, NULL, NULL);
	elm_list_item_append(stack, "Hi!", NULL, NULL, NULL, NULL);
	elm_list_item_append(stack, "Hi!", NULL, NULL, NULL, NULL);
	elm_list_item_append(stack, "Hi!", NULL, NULL, NULL, NULL);
	elm_list_item_append(stack, "Hi!", NULL, NULL, NULL, NULL);

	//------------------- prepare operators
	ops_register_operators();

	//------------------- done!
	evas_object_show(win);

	elm_run();
	elm_shutdown();
	return 0;
}

void ui_register_operator(const char * name, int nprop,
		const char * prop_names[], bool poll(float props[]))
{
	ops[ops_used].name  = name;
	ops[ops_used].nprop = nprop;
	ops[ops_used].poll  = poll;

	int i;
	$_(table, elm_table_add(win));
	evas_object_size_hint_weight_set(table,
			EVAS_HINT_EXPAND, 0);
	evas_object_size_hint_fill_set(table,
			EVAS_HINT_FILL, EVAS_HINT_FILL);
	for (i=0; i<nprop; i++) {
		// label
		$_(label, elm_label_add(win));
		elm_object_text_set(label, prop_names[i]);
		elm_table_pack(table, label, 0, i, 1, 1);
		evas_object_show(label);

		// spinner
		$_(spinner, elm_spinner_add(win));
		evas_object_size_hint_weight_set(spinner,
				EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_fill_set(spinner,
				EVAS_HINT_FILL, EVAS_HINT_FILL);
		elm_spinner_min_max_set(spinner, -65536, 65536);
		elm_spinner_step_set(spinner, 0.1);
		elm_spinner_round_set(spinner, 0.001);
		elm_spinner_label_format_set(spinner, "%0.3f");
		elm_table_pack(table, spinner, 1, i, 1, 1);
		evas_object_show(spinner);
	}
	ops[ops_used].table = table;

	elm_menu_item_add(menu_node, menu_add, NULL, name,
		(void *)$(void, (int idx, void * $1, void * $2) {
			void * node_select_cb = $(void, () {
				$_(o, elm_menu_item_object_get(
						elm_list_selected_item_get(nodes)));
				Operator * op = evas_object_data_get(o, "ipu:operator");
				if (props_current) {
					elm_object_content_unset(props);
					evas_object_hide(props_current);
				}
				props_current = op->table;
				elm_object_content_set(props, props_current);
				evas_object_show(props_current);
			});

			$_(o, elm_menu_item_object_get(elm_list_item_append(nodes,
					ops[idx].name, NULL, NULL, node_select_cb, NULL)));

			evas_object_data_set(o, "ipu:operator", &ops[idx]);
			evas_object_data_set(o, "ipu:props",
					new(float, *ops[idx].nprop));

			elm_list_go(nodes);
		}), (void *)ops_used);

	ops_used++;
}

