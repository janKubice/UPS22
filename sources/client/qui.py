from pydoc import text
from tkinter import *
import tkinter
from client import Client

class Qui:
	def __init__(self, gui:Tk):
		""" Metoda, která se zavolá při inicializaci nového objektu.
		"""

		#klient
		self.client = Client(self)

		# vytvoření jednotlivých labelů a tlačítek
		self.title = Label()
		self.right_btn = Button()
		self.left_btn = Button()
		self.text = Label()
		self.text_2 = Label()
		self.text_points = Label()
		self.text_another = Label()
		self.text_input = Entry()
		self.ip_input = Entry()
		self.port_input = Entry()
		self.text_wrong = Label()
		self.opt_selected = IntVar()
		self.gui:Tk = gui
		self.gui.update()

		self.name_input= Entry(textvariable='prezdivka')

		self.radio_buttons_array = []
		for i in range(4):
			radio_btn = Radiobutton(gui,text=" ",variable="", value = i+1,font = ("ariel",14))
			self.radio_buttons_array.append(radio_btn)


		self.display_title()
		self.display_menu()	
		self.gui.update()

	def show_q(self, question, answers):
		""" Zavolá potřebné metody pro zobrazení otázky a možných odpovědí a nadefinuje vzhled obrazovky.
		@param question = otázka
		@param answers = odpovědi
		"""

		self.text_points.config(text="Body: ", width=60,font=( 'ariel' ,16, 'bold' ), anchor= 'w' )
		self.text_points.place(x=200,y=200)
		self.right_btn.config(width=0, text='')
		self.right_btn.place(x=-1000, y=-1000)
		self.text_another.config(text='')
		self.text.config(text='')
		self.display_question(question)
		self.opt_selected=IntVar()
		self.radio_buttons()
		self.display_options(answers)
		self.buttons()


	def send_answer_btn(self):
		""" Odešlě odpověď na server.
		"""
		self.client.send_quiz_answer(self.opt_selected.get())

	def start_btn(self):
		""" Při spuštění hry vyžadá id pro hráče na serveru.
		"""
		self.client.request_id_player()

	def show_wrong_label(self, txt):
		""" Ukáže chybovou hlášku.
		@param txt = chybová hláška
		"""
		text_wrong = Label(self.gui, text=txt, width=self.gui.winfo_width(), font= ( 'ariel' ,16, 'bold' ), bg="red")
		text_wrong.pack()
		self.gui.after(5000, lambda: text_wrong.destroy())

	def buttons(self):
		""" Tlačítka pro uložení odpovědi a ukončení hry.
		"""
		self.right_btn.config(text="Ulozit odpoved",command=self.send_answer_btn, width=22, bg="red",fg="white",font=("ariel",16,"bold"))
		self.right_btn.place(x=350,y=300)

		quit_button = Button(self.gui, text="Quit", command=self.quit_game, width=5,bg="black", fg="white",font=("ariel",16," bold"))

		quit_button.place(x=700,y=50)

	def quit_game(self):
		""" Ukončení hry.
		"""
		self.client.leave_game()
		self.client.soc.close()
		self.gui.destroy()
		self.gui.quit()

	def display_options(self, answers):
		""" Ukáže možné odpovědi.
		"""
		print(f"odpovedi: {answers}")
		val=0
		answers = answers.split('-')
		self.opt_selected.set(0)

		for idx, option in enumerate(answers):
			self.radio_buttons_array[idx]['text']=option
			val+=1

	def display_question(self, question):
		""" Zobrazí otázku.
		"""
		self.text.config(text=question, width=60,font=( 'ariel' ,16, 'bold' ), anchor= 'w' )
	
	def display_answer(self, answer, points):
		""" Zobrazí správnou odpověď a body.
		"""
		self.text.config(text=f"Správná odpověď: {answer}.", width=60,font=( 'ariel' ,16, 'bold' ), anchor= 'w' )
		self.text_points.config(text=f"Body: {points}.", width=60,font=( 'ariel' ,16, 'bold' ), anchor= 'w' )


	def display_menu(self):
		""" 
		Zobrazí menu.
		"""
		self.text = Label(self.gui, text="Zadej jméno", width=60, font= ( 'ariel' ,16, 'bold' ))
		self.name_input = Entry(self.gui, textvariable='prezdivka')

		self.text_2 = Label(self.gui, text="Zadej IP a port", width=60, font= ( 'ariel' ,16, 'bold' ))
		self.ip_input = Entry(self.gui, textvariable='IP')
		self.port_input = Entry(self.gui, textvariable='PORT')

		self.text_points.config(text="", width=60,font=( 'ariel' ,16, 'bold' ), anchor= 'w' )
		
		self.right_btn = Button(self.gui)
		self.right_btn.config(text="Připojit se na server", 
							  command= lambda: self.client.request_id_player(self.name_input.get(), self.ip_input.get(), self.port_input.get()),
							  width=20, height=1,bg="red",fg="white",font=("ariel",16,"bold"))
		
		self.text.pack()
		self.name_input.pack()
		self.text_2.pack()
		self.ip_input.pack()
		self.port_input.pack()
		self.right_btn.pack()

	def display_room(self, num_players, admin, id = ''):
		""" Zobrazí lobby.
		@param num_players = počet hráčů
		@param admin (boolean) = zda je nebo není admin
		@param id = id lobby, zobrazuje se jen adminovi
		"""

		self.text_input.destroy()
		self.name_input.destroy()
		self.right_btn.destroy()
		self.left_btn.destroy()
		self.text.destroy()
		self.text_another.destroy()

		self.text = Label(self.gui)

		if admin:
			self.title.config(text=f'Místnost - ID {id}')
			self.text.config(text=f"Players: {num_players}/3", width=60, font=( 'ariel' ,16, 'bold' ), anchor= 'w' )
		else:
			self.title.config(text=f'Místnost')
			self.text.config(text=f"Room\nPlayers: {num_players}/3", width=60, font=( 'ariel' ,16, 'bold' ), anchor= 'w' )

		self.text.pack()

		self.text_points.config(text="Body: 0", width=60,font=( 'ariel' ,16, 'bold' ), anchor= 'w' )
		self.text_points.place(x=200,y=200)

		self.right_btn = Button(self.gui)
		self.text_another = Label(self.gui)

		if admin:
			self.right_btn.config(text="Start game",command=self.client.start_game, width=self.gui.winfo_width(),bg="red",fg="white",font=("ariel",16,"bold"))
			self.right_btn.pack()
		else:
			self.text_another.config(text="Nyní počkej na zahájení hry adminem.", width=60, font=( 'ariel' ,22, 'bold' ), anchor= 'w' )
			self.text_another.pack()


	def show_score(self, msg:str):
		""" Zobrazí větu zda vyhrál či nevyhrál uživatel.
		"""
		self.text.config(text=msg, width=60, font=( 'ariel' ,16, 'bold' ), anchor= 'w' )
		self.right_btn.config(text="Vrátit se do menu",command=self.back_to_menu, width=45,bg="red",fg="white",font=("ariel",16,"bold"))

	def back_to_menu(self):
		""" Vrátí do menu.
		"""
		for idx, btn in enumerate(self.radio_buttons_array):
			btn.config(text=" ",variable=self.opt_selected, value = idx+1,font = ("ariel",14))
			btn.place(x = 10000, y = 1000)
		self.display_menu()
		self.connect_input()

	def connect_input(self):
		""" Připojení do místnosti či vytvoření místnosti.
		"""
		self.ip_input.destroy()
		self.port_input.destroy()
		self.name_input.destroy()
		self.text_2.destroy()
		self.right_btn.destroy()
		self.text_input.destroy()

		entry_Var = tkinter.StringVar()

		self.text.config(text='Zadej ID mistnosti\n                   ')
		self.text_input = Entry(self.gui, textvariable=entry_Var)
		self.text_input.pack()

		self.right_btn = Button(self.gui)
		self.right_btn.config(text="Připojit se do místnosti",command= lambda: self.client.connect_to_game(entry_Var.get()), width=20, height=1,bg="purple",fg="white",font=("ariel",16,"bold"))
		self.right_btn.pack()

		self.left_btn = Button(self.gui)
		self.left_btn.config(text="Vytvořit místnost",command= lambda: self.client.create_new_room(), width=20, height=1,bg="purple",fg="white",font=("ariel",16,"bold"))
		self.left_btn.pack()

	def display_title(self):
		"""
			Zobrazení nadpisku v menu
		"""
		self.title = Label(self.gui, text="Menu", width=self.gui.winfo_width(), bg="blue",fg="white", font=("ariel", 20, "bold"))
		self.title.pack()

	def radio_buttons(self):
		""" Tlačítka ve stylu radio pro odpovědi.
		"""
		y_pos = 150
		
		for idx, btn in enumerate(self.radio_buttons_array):
			btn.config(text=" ",variable=self.opt_selected, value = idx+1,font = ("ariel",14))
			btn.place(x = 100, y = y_pos)
			y_pos += 40
		
	def close_window(self):
		self.quit_game()