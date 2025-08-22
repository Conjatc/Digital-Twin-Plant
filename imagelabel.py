import os
import tkinter as tk
from tkinter import filedialog
from PIL import Image, ImageTk

# --- CONFIG ---
image_extensions = (".jpg", ".jpeg", ".png")

# --- FUNCTIONS ---
def load_images(folder):
    return [os.path.join(folder, f) for f in os.listdir(folder)
            if f.lower().endswith(image_extensions)]

def show_image():
    global img_tk, current_image_path
    if index[0] < len(images):
        current_image_path = images[index[0]]
        img = Image.open(current_image_path)
        img.thumbnail((800, 600))
        img_tk = ImageTk.PhotoImage(img)
        panel.config(image=img_tk)
        status.config(text=f"{index[0]+1}/{len(images)}: {os.path.basename(current_image_path)}")
    else:
        status.config(text="All done!")
        panel.config(image="")
        good_btn.config(state="disabled")
        bad_btn.config(state="disabled")

def label_and_rename(label):
    old_path = current_image_path
    folder = os.path.dirname(old_path)
    filename = os.path.basename(old_path)
    new_filename = f"{label}.{filename}"
    new_path = os.path.join(folder, new_filename)
    os.rename(old_path, new_path)
    index[0] += 1
    images[index[0]-1] = new_path  # Update path in list (optional)
    show_image()

# --- MAIN ---
root = tk.Tk()
root.title("Image Labeler - Rename Mode")

# Ask folder
folder = filedialog.askdirectory(title="Select image folder")
if not folder:
    print("No folder selected.")
    root.destroy()
    exit()

images = load_images(folder)
index = [0]

panel = tk.Label(root)
panel.pack()

status = tk.Label(root, text="", font=("Arial", 12))
status.pack()

btn_frame = tk.Frame(root)
btn_frame.pack()

good_btn = tk.Button(btn_frame, text="Good_h", command=lambda: label_and_rename("good_h"), bg="green", fg="white", width=15)
good_btn.pack(side="left", padx=5)

bad_btn = tk.Button(btn_frame, text="Bad_h", command=lambda: label_and_rename("bad_h"), bg="red", fg="white", width=15)
bad_btn.pack(side="left", padx=5)

root.bind("g", lambda e: label_and_rename("good_h"))
root.bind("b", lambda e: label_and_rename("bad_h"))

show_image()
root.mainloop()
