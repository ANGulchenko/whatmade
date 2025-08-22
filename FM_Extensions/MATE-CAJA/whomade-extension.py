from gi.repository import Caja, GObject, Gtk
import subprocess

class WhoMadeExtension(GObject.GObject, Caja.MenuProvider):
    def __init__(self):
        pass

    def get_file_items(self, window, files):
        # Only show if one file is selected
        if len(files) != 1:
            return

        file = files[0]
        item = Caja.MenuItem(
            name="WhoMadeExtension::WhoMade",
            label="Who made this?",
            tip="Show which process created this file"
        )
        item.connect("activate", self.menu_activate_cb, file)
        return [item]

    def menu_activate_cb(self, menu, file):
        filepath = file.get_location().get_path()

        # Call my daemon
        try:
            result = subprocess.check_output(
                ["whomade", "-w", filepath], text=True
            ).strip()
            result = result[result.find('name:')+5:]
        except Exception as e:
            result = f"Error: {e}"

        # GTK dialog
        dialog = Gtk.MessageDialog(
            transient_for=None,
            flags=0,
            message_type=Gtk.MessageType.INFO,
            buttons=Gtk.ButtonsType.OK,
            text="Who made this file?",
        )
        dialog.format_secondary_text(f"{filepath}\n\nCreated by: {result}")
        dialog.run()
        dialog.destroy()
