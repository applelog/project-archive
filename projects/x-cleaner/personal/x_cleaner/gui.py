"""Native local UI on top of the existing, tested CLI safety engine."""
import json
import queue
import threading
import time
import webbrowser
from datetime import datetime
from pathlib import Path
from uuid import uuid4
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

from .api import XAPI
from .archive import read_archive
from .auth import Settings, TokenManager, authenticate
from .app_settings import save_profile
from .cli import http_client
from .common import CleanerError, PROJECT_ROOT, input_id, lock
from .job import Job
from .workflow import TERMINAL, confirmation, make_plan, run, verify

LABELS = {"post": "게시물", "reply": "댓글", "repost": "리포스트"}


def preview_selection(archive, selected, keep_text):
    by_id = {r["id"]: r for r in archive["records"]}
    if not selected or any(i not in by_id for i in selected):
        raise CleanerError("목록에서 삭제할 항목을 직접 선택하세요.")
    return {"version": 1, "archive": archive,
            "selections": [{"kind": by_id[i]["kind"], "id": i} for i in sorted(selected)],
            "keep": sorted({input_id(v) for v in keep_text.replace(",", "\n").splitlines() if v.strip()})}


def pending_count(job, retry_failed=False):
    plan = job.plan()
    return sum(job.state(a["key"])["status"] not in TERMINAL and
               (retry_failed or job.state(a["key"])["status"] not in {"failed", "uncertain"})
               for a in plan["actions"])


class UIAPI(XAPI):
    def __init__(self, client, tokens, stop, notify):
        self.stop = stop
        super().__init__(client, tokens, sleep=self.pause, notify=notify)

    def pause(self, seconds):
        if self.stop.wait(seconds):
            raise CleanerError("중단 요청을 받았습니다. 작업 기록을 저장했습니다.")

    def _send(self, method, path, token, params=None):
        if self.stop.is_set():
            raise CleanerError("중단 요청을 받아 다음 API 요청을 차단했습니다.")
        return super()._send(method, path, token, params)


class CleanerApp:
    def __init__(self, root):
        self.root = root
        self.events, self.stop = queue.Queue(), threading.Event()
        self.busy, self.archive, self.selected, self.job_path = False, None, set(), None
        self.draft_revision, self.saved_revision = 0, None
        self.buttons = []
        self.input_widgets = []
        try:
            self.settings = Settings.load()
        except CleanerError:
            self.settings = Settings("")
        root.title("X Cleaner — 개인용")
        root.geometry("1080x790")
        root.minsize(800, 660)
        root.protocol("WM_DELETE_WINDOW", self.close)
        shell = ttk.Frame(root, padding=22)
        shell.pack(fill="both", expand=True)
        ttk.Label(shell, text="X Cleaner", font=("Avenir Next", 23, "bold")).pack(anchor="w")
        ttk.Label(shell, text="내 기록을, 내가 고른 만큼.   선택한 내 게시물·댓글 삭제 / 내 리포스트 취소",
                  foreground="#65728c").pack(anchor="w", pady=(5, 18))
        tabs = ttk.Notebook(shell)
        tabs.pack(fill="both", expand=True)
        workspace, config = ttk.Frame(tabs, padding=16), ttk.Frame(tabs, padding=20)
        tabs.add(workspace, text="기록 선택 및 실행")
        tabs.add(config, text="API 설정 및 X 로그인")
        self.tabs = tabs
        self.config(config)
        self.workspace(workspace)
        self.status = tk.StringVar(value="API 설정은 앱에서 저장할 수 있습니다. 아카이브는 ‘ZIP 선택’ 또는 ‘폴더 선택’으로 읽으세요.")
        ttk.Label(shell, textvariable=self.status, wraplength=990, foreground="#455774").pack(anchor="w", pady=(14, 0))
        self.root.after(100, self.poll)

    def button(self, frame, text, action):
        button = ttk.Button(frame, text=text, command=action)
        self.buttons.append(button)
        return button

    def config(self, parent):
        ttk.Label(parent, text="앱 안에서 설정하고 로그인하세요", font=("Apple SD Gothic Neo", 20, "bold")).pack(anchor="w")
        ttk.Label(parent, text="X 개발자 앱의 Client ID는 최초 한 번 필요합니다. X 비밀번호와 브라우저 쿠키는 입력하지 않습니다.",
                  wraplength=850).pack(anchor="w", pady=(10, 18))
        grid = ttk.Frame(parent)
        grid.pack(fill="x")
        self.client_id = tk.StringVar(value=self.settings.client_id)
        self.client_secret = tk.StringVar(value="")
        self.redirect_uri = tk.StringVar(value=self.settings.redirect_uri)
        for row, (label, var, secret) in enumerate([
                ("Client ID", self.client_id, False),
                ("Client Secret (Native App에서는 빈 값)", self.client_secret, True),
                ("OAuth 콜백 주소", self.redirect_uri, False)]):
            ttk.Label(grid, text=label).grid(row=row, column=0, sticky="w", pady=12, padx=(0, 20))
            entry = ttk.Entry(grid, textvariable=var, show="•" if secret else "")
            entry.grid(row=row, column=1, sticky="ew", pady=12)
            self.input_widgets.append(entry)
        grid.columnconfigure(1, weight=1)
        self.clear_secret = tk.BooleanVar(value=False)
        ttk.Checkbutton(parent, text="저장된 Client Secret 지우기 (빈 입력은 기존 값을 유지)", variable=self.clear_secret).pack(anchor="w", pady=8)
        row = ttk.Frame(parent)
        row.pack(anchor="w", pady=(12, 16))
        self.button(row, "설정 저장", self.save_settings).pack(side="left", padx=(0, 10))
        self.button(row, "X 공식 화면에서 로그인", self.login).pack(side="left")
        self.button(row, "개발자 콘솔 열기", lambda: webbrowser.open("https://console.x.com")).pack(side="left", padx=10)
        self.config_status = tk.StringVar(value="저장된 설정이 있습니다." if self.settings.client_id else "Client ID를 입력하고 설정 저장을 누르세요.")
        ttk.Label(parent, textvariable=self.config_status, wraplength=850, foreground="#455774").pack(anchor="w", pady=8)
        ttk.Label(parent, text="설정 방법\n\n1. X Developer Console에서 OAuth 2.0 앱을 만드세요.\n"
                  "2. 개인용은 Native App을 선택하고 위 콜백 주소를 정확히 등록하세요.\n"
                  "3. Client ID를 복사해 저장한 뒤 ‘X 공식 화면에서 로그인’을 누르세요.\n\n"
                  "설정과 토큰은 이 Mac의 개인용 폴더에만 저장하며 파일 권한을 600으로 제한합니다.\n"
                  "X API 조회·삭제는 계정의 접근 권한과 크레딧 설정이 필요할 수 있습니다.",
                  wraplength=870).pack(anchor="w", pady=20)

    def workspace(self, parent):
        row = ttk.Frame(parent)
        row.pack(fill="x")
        self.button(row, "ZIP 선택", lambda: self.choose(False)).pack(side="left")
        self.button(row, "폴더 선택", lambda: self.choose(True)).pack(side="left", padx=8)
        self.button(row, "예시로 체험", self.demo).pack(side="left")
        self.button(row, "이전 작업 열기", self.resume).pack(side="right")
        self.archive_label = tk.StringVar(value="아카이브를 선택하면 게시물 목록이 나타납니다. 기본 선택은 모두 꺼져 있습니다.")
        ttk.Label(parent, textvariable=self.archive_label, wraplength=900).pack(anchor="w", pady=14)
        filters = ttk.Frame(parent)
        filters.pack(fill="x", pady=(0, 10))
        self.search, self.kind = tk.StringVar(), tk.StringVar(value="전체")
        ttk.Label(filters, text="검색").pack(side="left")
        ttk.Entry(filters, textvariable=self.search, width=40).pack(side="left", padx=8)
        combo = ttk.Combobox(filters, textvariable=self.kind, values=["전체"] + list(LABELS.values()), state="readonly", width=12)
        combo.pack(side="left")
        self.search.trace_add("write", lambda *_: self.render())
        self.kind.trace_add("write", lambda *_: self.render())
        self.button(filters, "선택 해제", self.unselect).pack(side="right")
        table = ttk.Frame(parent)
        table.pack(fill="both", expand=True)
        self.tree = ttk.Treeview(table, columns=("select", "kind", "text", "id"), show="headings", selectmode="browse")
        for key, title, width in [("select", "선택", 50), ("kind", "종류", 80), ("text", "내용", 530), ("id", "ID", 180)]:
            self.tree.heading(key, text=title)
            self.tree.column(key, width=width, stretch=key == "text")
        scroll = ttk.Scrollbar(table, orient="vertical", command=self.tree.yview)
        self.tree.configure(yscrollcommand=scroll.set)
        self.tree.pack(side="left", fill="both", expand=True)
        scroll.pack(side="right", fill="y")
        self.tree.bind("<ButtonRelease-1>", self.toggle_mouse)
        self.tree.bind("<space>", self.toggle_key)
        keep_row = ttk.Frame(parent)
        keep_row.pack(fill="x", pady=(12, 8))
        ttk.Label(keep_row, text="추가 보호 ID / 링크 (쉼표 또는 줄바꿈)").pack(side="left")
        self.keep = tk.StringVar()
        keep_entry = ttk.Entry(keep_row, textvariable=self.keep)
        keep_entry.pack(side="left", fill="x", expand=True, padx=10)
        self.input_widgets.append(keep_entry)
        self.keep.trace_add("write", lambda *_: self.invalidate())
        self.counts = tk.StringVar(value="선택 0개")
        ttk.Label(parent, textvariable=self.counts, foreground="#315eea").pack(anchor="w", pady=5)
        self.retry_failed = tk.BooleanVar(value=False)
        retry_box = ttk.Checkbutton(parent, text="실패·미확정 항목도 명시적으로 재시도 (결과를 먼저 확인하세요)", variable=self.retry_failed)
        retry_box.pack(anchor="w", pady=4)
        self.input_widgets.append(retry_box)
        actions = ttk.Frame(parent)
        actions.pack(fill="x", pady=(8, 0))
        for title, fn in [("1. 미리보기 저장", self.preview), ("2. 계정·대상 검증", self.verify), ("3. 최종 확인 후 실행", self.execute), ("보고서 폴더 열기", self.report)]:
            self.button(actions, title, fn).pack(side="left", padx=(0, 8))
        ttk.Button(actions, text="현재 요청 후 중단", command=self.stop.set).pack(side="right")

    def save_settings(self):
        if self.busy:
            return False
        try:
            previous = self.settings.client_secret if self.client_id.get().strip() == self.settings.client_id else ""
            secret = "" if self.clear_secret.get() else self.client_secret.get() or previous
            self.settings = save_profile(self.client_id.get(), secret, self.redirect_uri.get())
            self.client_secret.set("")
            self.clear_secret.set(False)
            self.config_status.set("설정을 저장했습니다. X 공식 화면에서 로그인하세요.")
            return True
        except CleanerError as error:
            messagebox.showerror("설정 확인", str(error), parent=self.root)
            return False

    def notify(self, text):
        if not text.startswith("https://"):
            self.events.put(("status", text))

    def submit(self, work, done):
        if self.busy:
            return
        self.busy = True
        self.stop.clear()
        for button in self.buttons + self.input_widgets:
            button.configure(state="disabled")
        def worker():
            try:
                result = work()
                self.events.put(("done", lambda: done(result)))
            except CleanerError as error:
                self.events.put(("error", str(error)))
            except Exception:
                self.events.put(("error", "파일 또는 작업을 처리하지 못했습니다. 비밀값 보호를 위해 원문 오류는 표시하지 않습니다."))
        threading.Thread(target=worker, daemon=True).start()

    def poll(self):
        while not self.events.empty():
            event, value = self.events.get_nowait()
            if event == "status":
                self.status.set(value)
            else:
                self.busy = False
                for button in self.buttons + self.input_widgets:
                    button.configure(state="normal")
                if event == "done":
                    value()
                else:
                    self.status.set(value)
                    messagebox.showerror("작업 중단", value, parent=self.root)
        self.root.after(100, self.poll)

    def login(self):
        if not self.save_settings():
            return
        try:
            self.settings.validate()
        except CleanerError as error:
            messagebox.showerror("API 설정 필요", str(error), parent=self.root)
            return
        settings = self.settings
        def work():
            with http_client() as client:
                authenticate(settings, client, notify=self.notify)
        self.status.set("X 공식 승인 화면을 엽니다. 해당 화면에서만 로그인하세요.")
        self.submit(work, lambda _: self.config_status.set("X 승인을 완료했습니다. 기록 선택 탭에서 미리보기와 계정 검증을 진행하세요."))

    def choose(self, folder):
        path = filedialog.askdirectory(parent=self.root) if folder else filedialog.askopenfilename(parent=self.root, filetypes=[("X 아카이브 ZIP", "*.zip")])
        if path:
            self.submit(lambda: read_archive(path), self.loaded)

    def demo(self):
        self.submit(lambda: read_archive(PROJECT_ROOT / "examples/archive"), self.loaded)

    def loaded(self, archive):
        self.archive, self.selected, self.job_path = archive, set(), None
        self.invalidate()
        self.archive_label.set("@{} · 전체 {}개 · 파일을 읽었습니다. 삭제할 항목의 선택 칸을 누르세요.".format(archive["username"] or "계정 정보 없음", len(archive["records"])))
        self.render()

    def invalidate(self):
        self.draft_revision += 1
        self.refresh_counts()

    def render(self):
        if not hasattr(self, "tree"):
            return
        self.tree.delete(*self.tree.get_children())
        if not self.archive:
            return
        search, kind = self.search.get().casefold(), self.kind.get()
        for record in self.archive["records"]:
            if (kind != "전체" and LABELS[record["kind"]] != kind) or search not in (record["text"] + record["id"]).casefold():
                continue
            self.tree.insert("", "end", iid=record["id"], values=("✓" if record["id"] in self.selected else "", LABELS[record["kind"]], record["text"].replace("\n", " "), record["id"]))

    def toggle_mouse(self, event):
        if self.tree.identify_column(event.x) == "#1":
            self.toggle(self.tree.identify_row(event.y))

    def toggle_key(self, _):
        self.toggle(self.tree.focus())
        return "break"

    def toggle(self, identifier):
        if self.busy or not identifier:
            return
        if identifier in self.selected:
            self.selected.remove(identifier)
        else:
            self.selected.add(identifier)
        self.tree.set(identifier, "select", "✓" if identifier in self.selected else "")
        self.invalidate()

    def unselect(self):
        self.selected.clear()
        self.invalidate()
        self.render()

    def refresh_counts(self):
        if not hasattr(self, "counts"):
            return
        total = len(self.archive["records"]) if self.archive else 0
        self.counts.set("전체 {}개  /  선택 {}개  /  선택 밖 {}개  ·  선택 변경 후 미리보기를 다시 저장하세요.".format(total, len(self.selected), total - len(self.selected)))

    def preview(self):
        if not self.archive:
            messagebox.showinfo("아카이브 선택", "먼저 ZIP 또는 폴더를 선택하세요.", parent=self.root)
            return
        try:
            inputs = preview_selection(self.archive, self.selected, self.keep.get())
        except CleanerError as error:
            messagebox.showerror("선택 확인", str(error), parent=self.root)
            return
        path = "jobs/gui-{}-{}".format(datetime.now().strftime("%Y%m%d-%H%M%S"), uuid4().hex[:8])
        revision = self.draft_revision
        def work():
            job = Job.create(path, inputs)
            with job.open():
                job.save_plan(make_plan(inputs))
                return job.reports()
        def done(summary):
            self.job_path, self.saved_revision = path, revision
            self.show_summary(summary)
        self.submit(work, done)

    def checked_job(self):
        if not self.job_path or self.saved_revision != self.draft_revision:
            raise CleanerError("현재 선택·보존 목록의 미리보기를 먼저 저장하세요.")
        return Job(self.job_path)

    def operate(self, job, operation):
        with job.open():
            try:
                with lock(".secrets/token.lock"), http_client() as client:
                    api = UIAPI(client, TokenManager(self.settings, client), self.stop, self.notify)
                    return operation(job, api)
            finally:
                job.reports()

    def verify(self):
        try:
            job = self.checked_job()
            self.settings.validate()
        except CleanerError as error:
            messagebox.showerror("먼저 확인하세요", str(error), parent=self.root)
            return
        if not messagebox.askokcancel("X API 조회", "인증 계정과 선택한 항목을 공식 API로 조회합니다. 비용이 발생할 수 있습니다. 삭제 요청은 보내지 않습니다.", parent=self.root):
            return
        self.submit(lambda: self.operate(job, verify), self.show_summary)

    def execute(self):
        try:
            job = self.checked_job()
            with job.open():
                plan = job.plan()
                if not plan["verified"] or plan["blocks"]:
                    raise CleanerError("계정·대상 검증을 먼저 완료하고 차단 사유를 확인하세요.")
                count = pending_count(job, self.retry_failed.get())
                if not count:
                    raise CleanerError("미완료 작업이 없습니다. 실패·미확정 항목은 보고서를 확인하세요.")
                phrase = confirmation(job, plan, count)
                user_phrase = "선택한 {}개 실행".format(count)
                owner = job.inputs()["archive"]["owner_id"]
        except CleanerError as error:
            messagebox.showerror("실행 차단", str(error), parent=self.root)
            return
        dialog = tk.Toplevel(self.root)
        dialog.title("최종 확인")
        dialog.transient(self.root)
        dialog.grab_set()
        frame = ttk.Frame(dialog, padding=24)
        frame.pack(fill="both", expand=True)
        ttk.Label(frame, text="계정 {}의 선택된 미완료 {}개를 실행합니다.\n게시물·댓글 삭제는 되돌릴 수 없습니다.\n아래 확인 문구를 정확히 입력하세요.".format(owner, count)).pack(anchor="w")
        ttk.Label(frame, text=user_phrase, font=("Apple SD Gothic Neo", 15, "bold")).pack(pady=16)
        value = tk.StringVar()
        entry = ttk.Entry(frame, textvariable=value, width=50)
        entry.pack(fill="x")
        entry.focus_set()
        def start():
            if value.get() != user_phrase:
                return
            supplied = phrase  # Bind the human confirmation to the engine's frozen plan hash.
            retry_failed = self.retry_failed.get()
            dialog.destroy()
            self.submit(lambda: self.operate(job, lambda j, api: run(j, api, resume=True, retry_failed=retry_failed, confirm=lambda _: supplied, notify=self.notify)), self.show_summary)
        execute = ttk.Button(frame, text="확인한 선택 항목 실행", command=start, state="disabled")
        execute.pack(pady=(20, 0))
        value.trace_add("write", lambda *_: execute.configure(state="normal" if value.get() == user_phrase else "disabled"))

    def resume(self):
        path = filedialog.askdirectory(parent=self.root, initialdir=PROJECT_ROOT / "jobs", title="이전 작업 폴더 선택")
        if not path:
            return
        try:
            job = Job(path)
            with job.open():
                inputs, summary, plan = job.inputs(), job.reports(), job.plan()
            self.archive = inputs["archive"]
            self.selected = set(plan["selected_ids"])
            self.keep.set("\n".join(inputs["keep"]))
            self.job_path, self.saved_revision = str(job.path), self.draft_revision
            self.archive_label.set("이전 작업을 열었습니다. 저장된 선택 목록을 복원했습니다.")
            self.render()
            self.show_summary(summary)
        except CleanerError as error:
            messagebox.showerror("이어하기 차단", str(error), parent=self.root)

    def show_summary(self, summary):
        self.status.set("전체 {}개 / 선택 {}개 / 보존 {}개 / 실행 대상 {}개 / 검증 {}. {}".format(
            summary["archive_total"], summary["selected_archive_count"], summary["preserved_archive_count"], summary["delete_target_count"],
            "완료" if summary["verified"] else "미완료", " · ".join(summary["blocks"]) or json.dumps(summary["status_counts"], ensure_ascii=False)))

    def report(self):
        if self.job_path:
            webbrowser.open(Job(self.job_path).path.as_uri())

    def close(self):
        if self.busy:
            self.stop.set()
            messagebox.showinfo("중단 요청", "현재 요청을 마치고 기록을 저장할 때까지 기다려 주세요. X 승인 대기는 최대 5분 후 종료됩니다.", parent=self.root)
            return
        self.root.destroy()


def launch():
    root = tk.Tk()
    CleanerApp(root)
    root.mainloop()
