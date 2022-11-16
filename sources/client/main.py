from tkinter import Tk
from qui import Qui
import settings as sett

if __name__ == '__main__':
    gui = Tk()

    gui.geometry(sett.resolution)
    gui.title(sett.title)

    quiz_qui = Qui(gui)
    gui.protocol("WM_DELETE_WINDOW", quiz_qui.close_window)

    gui.mainloop()