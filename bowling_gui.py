import tkinter as tk
from tkinter import messagebox
import subprocess
import os
import re

C_SOURCE_DIR = os.path.dirname(os.path.abspath(__file__))
EXE_PATH = os.path.join(C_SOURCE_DIR, "score_calculator.exe")
C_FILES = ["linked_list.c", "queue.c", "tpb.c", "score_calculator.c"]

DARK   = "#1a1a2e"
MID    = "#16213e"
BLUE   = "#0f3460"
ACCENT = "#e94560"
GREEN  = "#1b5e20"
TEXT   = "#eaeaea"
HINT   = "#666688"
FONT     = ("Segoe UI", 10)
FONT_BIG = ("Segoe UI", 12, "bold")
MONO     = ("Consolas", 11, "bold")
CELL_W   = 56
CELL10_W = 80
CELL_H1  = 28
CELL_H2  = 32
STRIKE_COL = "#ffd54f"
SPARE_COL  = "#81d4fa"
BORDER     = "#445"

# ── compile / run ──────────────────────────────────────────────
def compile_c():
    sources = [os.path.join(C_SOURCE_DIR, f) for f in C_FILES]
    cmd = ["gcc", "-Wall", "-ansi", "-pedantic-errors", "-o", EXE_PATH] + sources
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        messagebox.showerror("Compile Error", r.stderr)
        return False
    return True

def run_game(game_string):
    if not os.path.exists(EXE_PATH):
        if not compile_c():
            return None
    r = subprocess.run([EXE_PATH, game_string], capture_output=True, text=True)
    if r.returncode != 0 or not r.stdout.strip():
        return None
    return r.stdout.strip()

def parse_output(raw):
    if not raw:
        return None, []
    m = re.match(r'^(.*?):\s*(.+)$', raw.strip())
    if not m:
        return None, []
    scores = list(map(int, m.group(2).split()))
    return m.group(1).strip(), scores

# ── game state ─────────────────────────────────────────────────
rolls = []

def get_frame_index():
    """Return (current_frame 0-9, roll_in_frame 0/1/2, boundaries[])
    boundaries[f] = index in rolls[] where frame f starts."""
    boundaries = [0]
    i = 0
    frame = 0
    while frame < 9 and i < len(rolls):
        r = rolls[i]
        if r in ('x', 'X'):
            frame += 1
            i += 1
            boundaries.append(i)
        else:
            if i + 1 < len(rolls):
                frame += 1
                i += 2
                boundaries.append(i)
            else:
                break   # mid-frame, one roll pending
    current_frame = frame
    roll_in_frame = len(rolls) - boundaries[current_frame] if current_frame < len(boundaries) else 0
    return current_frame, roll_in_frame, boundaries

def available_buttons():
    cf, rif, boundaries = get_frame_index()
    if cf > 9:
        return set()

    # ── frames 0-8 ──
    if cf < 9:
        if rif == 0:
            # First roll: anything except spare
            return {'x', '-', 'f', '1','2','3','4','5','6','7','8','9'}
        else:
            # Second roll: spare always valid; digits limited by first roll
            first = rolls[boundaries[cf]]
            pv = int(first) if first.isdigit() else 0
            av = {'-', 'f', '/'}
            for d in range(1, 10 - pv):
                av.add(str(d))
            return av

    # ── 10th frame ──
    r10 = rolls[boundaries[9]:] if len(boundaries) > 9 else []
    n = len(r10)

    if n == 0:
        return {'x', '-', 'f', '1','2','3','4','5','6','7','8','9'}
    elif n == 1:
        r0 = r10[0]
        if r0 in ('x', 'X'):
            return {'x', '-', 'f', '1','2','3','4','5','6','7','8','9'}
        pv = int(r0) if r0.isdigit() else 0
        av = {'-', 'f', '/'}
        for d in range(1, 10 - pv):
            av.add(str(d))
        return av
    elif n == 2:
        r0, r1 = r10[0], r10[1]
        if r0 in ('x', 'X'):
            if r1 in ('x', 'X'):
                # X X _ : third can be anything
                return {'x', '-', 'f', '1','2','3','4','5','6','7','8','9'}
            else:
                # X d _ : third is limited by second roll
                pv = int(r1) if r1.isdigit() else 0
                av = {'-', 'f', '/'}
                for d in range(1, 10 - pv):
                    av.add(str(d))
                return av
        elif r1 == '/':
            # d / _ : third is a fresh ball
            return {'x', '-', 'f', '1','2','3','4','5','6','7','8','9'}
        else:
            return set()  # two non-special rolls: game over
    else:
        return set()

def is_game_complete():
    cf, _, boundaries = get_frame_index()
    if cf < 9:
        return False
    r10 = rolls[boundaries[9]:] if len(boundaries) > 9 else []
    n = len(r10)
    if n < 2:
        return False
    r0, r1 = r10[0], r10[1]
    if r0 in ('x', 'X') or r1 == '/':
        return n >= 3
    return n >= 2

# ── scorecard data ─────────────────────────────────────────────
frame_rolls  = [[] for _ in range(10)]
frame_scores = [""] * 10

def build_frame_rolls_display():
    for i in range(10):
        frame_rolls[i] = []
    _, _, boundaries = get_frame_index()
    for fi in range(10):
        if fi >= len(boundaries):
            break
        start = boundaries[fi]
        end = boundaries[fi + 1] if (fi + 1) < len(boundaries) else len(rolls)
        frame_rolls[fi] = [r.upper() if r in ('x', 'f') else r for r in rolls[start:end]]

def refresh_scores():
    global frame_scores
    frame_scores = [""] * 10
    if not rolls:
        return
    raw = run_game("".join(rolls))
    if not raw:
        return
    _, scores = parse_output(raw)
    for i, s in enumerate(scores):
        if i < 10:
            frame_scores[i] = str(s)

def add_roll(token):
    rolls.append(token)
    build_frame_rolls_display()
    refresh_scores()
    update_scorecard()
    update_buttons()
    status_var.set("Game: " + "".join(rolls))

def undo_roll():
    if rolls:
        rolls.pop()
    build_frame_rolls_display()
    refresh_scores()
    update_scorecard()
    update_buttons()
    status_var.set("Game: " + "".join(rolls) if rolls else "Ready.")

def clear_all():
    rolls.clear()
    for i in range(10):
        frame_rolls[i] = []
        frame_scores[i] = ""
    update_scorecard()
    update_buttons()
    status_var.set("Ready.")

# ── scorecard canvas ───────────────────────────────────────────
def draw_scorecard():
    c = scorecard_canvas
    c.delete("all")
    x, y = 2, 2

    for fi in range(10):
        w = CELL10_W if fi == 9 else CELL_W
        c.create_rectangle(x, y, x+w, y+CELL_H1+CELL_H2, outline=BORDER, fill=MID, width=1)
        c.create_text(x+4, y+4, text=str(fi+1), anchor="nw",
                      font=("Segoe UI", 7), fill=HINT)

        fr = frame_rolls[fi]

        if fi < 9:
            bw = 18
            bx1 = x + w - bw*2 - 3
            bx2 = x + w - bw - 1
            for bx in (bx1, bx2):
                c.create_rectangle(bx, y+1, bx+bw, y+CELL_H1-1,
                                   outline=BORDER, fill=BLUE, width=1)
            r1 = fr[0].upper() if len(fr) > 0 else ""
            r2 = fr[1].upper() if len(fr) > 1 else ""
            col1 = STRIKE_COL if r1 == "X" else TEXT
            col2 = SPARE_COL  if r2 == "/" else (STRIKE_COL if r2 == "X" else TEXT)
            c.create_text(bx1+bw//2, y+CELL_H1//2, text=r1, font=MONO, fill=col1, anchor="center")
            c.create_text(bx2+bw//2, y+CELL_H1//2, text=r2, font=MONO, fill=col2, anchor="center")
        else:
            bw = 22
            bx1, bx2, bx3 = x+3, x+3+bw+1, x+3+(bw+1)*2
            for bx in (bx1, bx2, bx3):
                c.create_rectangle(bx, y+1, bx+bw, y+CELL_H1-1,
                                   outline=BORDER, fill=BLUE, width=1)
            labels = [fr[i].upper() if i < len(fr) else "" for i in range(3)]
            cols   = [STRIKE_COL if l=="X" else (SPARE_COL if l=="/" else TEXT) for l in labels]
            for bx, lbl, col in zip((bx1,bx2,bx3), labels, cols):
                c.create_text(bx+bw//2, y+CELL_H1//2, text=lbl, font=MONO, fill=col, anchor="center")

        score_y = y + CELL_H1
        score_bg = GREEN if frame_scores[fi] else MID
        c.create_rectangle(x+1, score_y, x+w-1, score_y+CELL_H2-1, outline="", fill=score_bg)
        c.create_text(x+w//2, score_y+CELL_H2//2,
                      text=frame_scores[fi],
                      font=("Consolas", 12, "bold"), fill=TEXT, anchor="center")
        x += w

    if frame_scores[9]:
        c.create_text(x+8, y+CELL_H1+CELL_H2//2,
                      text=f"= {frame_scores[9]}",
                      font=("Segoe UI", 11, "bold"), fill=ACCENT, anchor="w")

def update_scorecard():
    draw_scorecard()

def update_buttons():
    avail = available_buttons()
    for token, btn in roll_buttons.items():
        if token in avail:
            btn.config(state="normal", bg=BLUE)
        else:
            btn.config(state="disabled", bg="#0a1e3a")
    cf, _, _ = get_frame_index()
    for i, lbl in enumerate(frame_indicators):
        if i < cf:
            lbl.config(fg="#4CAF50")
        elif i == cf and cf < 10:
            lbl.config(fg=ACCENT)
        else:
            lbl.config(fg=HINT)

def on_recompile():
    status_var.set("Compiling...")
    root.update()
    status_var.set("Compiled." if compile_c() else "Compile failed.")

# ── UI ─────────────────────────────────────────────────────────
root = tk.Tk()
root.title("Bowling Score Calculator")
root.resizable(False, False)
root.configure(bg=DARK)

tk.Label(root, text="🎳 Bowling Score Calculator",
         font=("Segoe UI", 15, "bold"), bg=DARK, fg=TEXT).pack(pady=(16,2))
tk.Label(root, text="Press the buttons to enter each roll",
         font=FONT, bg=DARK, fg=HINT).pack(pady=(0,8))

fi_outer = tk.Frame(root, bg=DARK)
fi_outer.pack()
frame_indicators = []
for i in range(10):
    w = CELL10_W if i == 9 else CELL_W
    lbl = tk.Label(fi_outer, text=str(i+1), font=("Segoe UI", 8, "bold"),
                   bg=DARK, fg=HINT, width=w//8)
    lbl.pack(side="left", padx=1)
    frame_indicators.append(lbl)

total_w = CELL_W*9 + CELL10_W + 70
scorecard_canvas = tk.Canvas(root, width=total_w, height=CELL_H1+CELL_H2+6,
                              bg=DARK, highlightthickness=0)
scorecard_canvas.pack(padx=16, pady=6)

btn_outer = tk.Frame(root, bg=DARK)
btn_outer.pack(pady=4)

BUTTON_DEFS = [("Strike (X)","x"), ("Spare (/)","/" ), ("Gutter (-)","- "), ("Fault (F)","f")]
for d in range(1, 10):
    BUTTON_DEFS.append((str(d), str(d)))

roll_buttons = {}
top_row = tk.Frame(btn_outer, bg=DARK)
top_row.pack(pady=(0,4))
for label, token in BUTTON_DEFS[:4]:
    t = token.strip()
    btn = tk.Button(top_row, text=label, font=FONT, bg=BLUE, fg=TEXT,
                    relief="flat", cursor="hand2", padx=10, pady=6, width=10,
                    command=lambda tt=t: add_roll(tt))
    btn.pack(side="left", padx=4)
    roll_buttons[t] = btn

digit_row = tk.Frame(btn_outer, bg=DARK)
digit_row.pack()
for label, token in BUTTON_DEFS[4:]:
    btn = tk.Button(digit_row, text=label, font=FONT_BIG, bg=BLUE, fg=TEXT,
                    relief="flat", cursor="hand2", width=4, pady=6,
                    command=lambda t=token: add_roll(t))
    btn.pack(side="left", padx=4)
    roll_buttons[token] = btn

action_row = tk.Frame(root, bg=DARK)
action_row.pack(pady=6)
tk.Button(action_row, text="⟵ Undo", font=FONT, bg=MID, fg=TEXT,
          relief="flat", cursor="hand2", padx=10, pady=6,
          command=undo_roll).pack(side="left", padx=6)
tk.Button(action_row, text="Clear", font=FONT, bg=MID, fg=TEXT,
          relief="flat", cursor="hand2", padx=10, pady=6,
          command=clear_all).pack(side="left", padx=6)

bottom = tk.Frame(root, bg=BLUE)
bottom.pack(fill="x", pady=(8,0))
status_var = tk.StringVar(value="Ready.")
tk.Label(bottom, textvariable=status_var, font=("Segoe UI", 9),
         bg=BLUE, fg=HINT).pack(side="left", padx=8, pady=4)
tk.Button(bottom, text="Recompile", font=("Segoe UI", 9), bg=BLUE, fg=TEXT,
          relief="flat", cursor="hand2", command=on_recompile).pack(side="right", padx=8, pady=4)

draw_scorecard()
update_buttons()
root.mainloop()
