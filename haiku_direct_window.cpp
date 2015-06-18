#include "main/main.h"
#include "haiku_direct_window.h"

HaikuDirectWindow::HaikuDirectWindow(BRect p_frame)
   : BDirectWindow(p_frame, "Godot", B_TITLED_WINDOW, 0)
{
	last_mouse_pos_valid = false;
	last_buttons_state = 0;
	last_button_mask = 0;
}


HaikuDirectWindow::~HaikuDirectWindow() {
	if (update_runner) {
		delete update_runner;
	}
}

void HaikuDirectWindow::SetHaikuGLView(HaikuGLView* p_view) {
	view = p_view;
}

void HaikuDirectWindow::StartMessageRunner() {
	update_runner = new BMessageRunner(BMessenger(this),
		new BMessage(REDRAW_MSG), 1000000/60 /* 60 fps */);
}

void HaikuDirectWindow::StopMessageRunner() {
	delete update_runner;
}

void HaikuDirectWindow::SetInput(InputDefault* p_input) {
	input = p_input;
}

bool HaikuDirectWindow::QuitRequested() {
	view->EnableDirectMode(false);
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void HaikuDirectWindow::DirectConnected(direct_buffer_info* info) {
	view->DirectConnected(info);
	view->EnableDirectMode(true);
}

void HaikuDirectWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case REDRAW_MSG:
			//ERR_PRINT("iteration 1");
			Main::iteration();

			//if (NeedsUpdate()) {
			//	ERR_PRINT("NEEDS UPDATE");
			//	Main::force_redraw();
			//}

			//ERR_PRINT("iteration 2");
			break;

		case B_INVALIDATE:
			ERR_PRINT("WINDOW B_INVALIDATE");
			//Main::force_redraw();
			break;

		default:
			BDirectWindow::MessageReceived(message);
	}
}

void HaikuDirectWindow::DispatchMessage(BMessage* message, BHandler* handler) {
	switch (message->what) {
		case B_MOUSE_DOWN:
		case B_MOUSE_UP:
			DispatchMouseButton(message);
			break;

		case B_MOUSE_MOVED:
			DispatchMouseMoved(message);
			break;

		default:
			BDirectWindow::DispatchMessage(message, handler);
	}
}

void HaikuDirectWindow::DispatchMouseButton(BMessage* message) {
	message->PrintToStream();

	BPoint where;
	if (message->FindPoint("where", &where) != B_OK) {
		return;
	}

	uint32 modifiers = message->FindInt32("modifiers");
	uint32 buttons = message->FindInt32("buttons");
	uint32 button = buttons ^ last_buttons_state;
	last_buttons_state = buttons;

	// TODO: implement the mouse_mode checks
	//if (mouse_mode == MOUSE_MODE_CAPTURED) {
	//	event.xbutton.x=last_mouse_pos.x;
	//	event.xbutton.y=last_mouse_pos.y;
	//}

	InputEvent mouse_event;
	mouse_event.ID = ++event_id;
	mouse_event.type = InputEvent::MOUSE_BUTTON;
	mouse_event.device = 0;

	mouse_event.mouse_button.mod = GetKeyModifierState(modifiers);
	mouse_event.mouse_button.button_mask = GetMouseButtonState(buttons);
	mouse_event.mouse_button.x = where.x;
	mouse_event.mouse_button.y = where.y;
	mouse_event.mouse_button.global_x = where.x;
	mouse_event.mouse_button.global_y = where.y;

	switch (button) {
		default:
		case B_PRIMARY_MOUSE_BUTTON:
			mouse_event.mouse_button.button_index = 1;
			break;

		case B_SECONDARY_MOUSE_BUTTON:
			mouse_event.mouse_button.button_index = 2;
			break;

		case B_TERTIARY_MOUSE_BUTTON:
			mouse_event.mouse_button.button_index = 3;
			break;
	}

	mouse_event.mouse_button.pressed = (message->what == B_MOUSE_DOWN);

	if (message->what == B_MOUSE_DOWN && mouse_event.mouse_button.button_index == 1) {
		int32 clicks = message->FindInt32("clicks");

		if (clicks > 1) {
			mouse_event.mouse_button.doubleclick=true;
		}
	}

	input->parse_input_event(mouse_event);
}

void HaikuDirectWindow::DispatchMouseMoved(BMessage* message) {
	BPoint where;
	if (message->FindPoint("where", &where) != B_OK) {
		return;
	}

	Point2i pos(where.x, where.y);
	uint32 modifiers = message->FindInt32("modifiers");
	uint32 buttons = message->FindInt32("buttons");

	if (!last_mouse_pos_valid) {
		last_mouse_position = pos;
		last_mouse_pos_valid = true;
	}

	Point2i rel = pos - last_mouse_position;

	InputEvent motion_event;
	motion_event.ID = ++event_id;
	motion_event.type = InputEvent::MOUSE_MOTION;
	motion_event.device = 0;

	motion_event.mouse_motion.mod = GetKeyModifierState(modifiers);
	motion_event.mouse_motion.button_mask = GetMouseButtonState(buttons);
	motion_event.mouse_motion.x = pos.x;
	motion_event.mouse_motion.y = pos.y;
	input->set_mouse_pos(pos);
	motion_event.mouse_motion.global_x = pos.x;
	motion_event.mouse_motion.global_y = pos.y;
	motion_event.mouse_motion.speed_x = input->get_mouse_speed().x;
	motion_event.mouse_motion.speed_y = input->get_mouse_speed().y;

	motion_event.mouse_motion.relative_x = rel.x;
	motion_event.mouse_motion.relative_y = rel.y;

	last_mouse_position = pos;

	input->parse_input_event(motion_event);
}

inline InputModifierState HaikuDirectWindow::GetKeyModifierState(uint32 p_state) {
	InputModifierState state;

	state.shift = (p_state & B_SHIFT_KEY) != 0;
	state.control = (p_state & B_CONTROL_KEY) != 0;
	state.alt = (p_state & B_OPTION_KEY) != 0;
	state.meta = (p_state & B_COMMAND_KEY) != 0;

	return state;
}

inline int HaikuDirectWindow::GetMouseButtonState(uint32 p_state) {
	int state = 0;

	if (p_state & B_PRIMARY_MOUSE_BUTTON) {
		state |= 1 << 0;
	}

	if (p_state & B_SECONDARY_MOUSE_BUTTON) {
		state |= 1 << 1;
	}

	if (p_state & B_TERTIARY_MOUSE_BUTTON) {
		state |= 1 << 2;
	}

	last_button_mask = state;

	return state;
}
