from invoke import task
from pathlib import Path
import shutil
import re
import rcssmin
import rjsmin

from colorama import init, Fore #, Back, Style

# import subprocess

SRC_DIR = Path("src_www")
DEST_DIR = Path("data/www")

init(autoreset=True)


def log_info(msg):
    print(f"{Fore.CYAN}[INFO] {msg}")


def log_warn(msg):
    print(f"{Fore.YELLOW}[WARN] {msg}")


def log_error(msg):
    print(f"{Fore.RED}[ERROR] {msg}")


def log_step(msg):
    print(
        f"\n{Fore.GREEN}===================================================\n"
        f"  {msg}\n"
        f"===================================================\n"
    )


@task
def clean(ctx):
    """ Clean build folder """
    log_step("Cleaning build folder")
    if DEST_DIR.exists():
        shutil.rmtree(DEST_DIR)
        log_info(f"Cleaned {DEST_DIR}")
    else:
        log_info(f"{DEST_DIR} does not exist")


@task
def css_min(ctx):
    """ Minify CSS and add .min suffix """
    log_step("Minifying CSS files")
    css_files = SRC_DIR.glob("**/*.css")

    for css_file in css_files:
        css_content = css_file.read_text(encoding="utf-8")
        minified = rcssmin.cssmin(css_content)

        rel_path = css_file.relative_to(SRC_DIR)
        dest_file = DEST_DIR / rel_path.parent / f"{rel_path.stem}.min.css"
        dest_file.parent.mkdir(parents=True, exist_ok=True)

        dest_file.write_text(minified, encoding="utf-8")
        log_info(f"Minified {css_file} -> {dest_file}")


@task
def js_min(ctx):
    """ Minify JS and add .min suffix """
    log_step("Minifying JS files")
    js_files = SRC_DIR.glob("**/*.js")

    for js_file in js_files:
        js_content = js_file.read_text(encoding="utf-8")
        minified = rjsmin.jsmin(js_content)

        rel_path = js_file.relative_to(SRC_DIR)
        dest_file = DEST_DIR / rel_path.parent / f"{rel_path.stem}.min.js"
        dest_file.parent.mkdir(parents=True, exist_ok=True)

        dest_file.write_text(minified, encoding="utf-8")
        log_info(f"Minified {js_file} -> {dest_file}")


@task
def images(ctx):
    """ Copy images """
    log_step("Copying images")
    image_extensions = {".svg", ".png", ".jpg", ".jpeg", ".gif", ".ico", ".webp"}

    for src_file in SRC_DIR.rglob("*"):
        if src_file.is_file() and src_file.suffix.lower() in image_extensions:
            rel_path = src_file.relative_to(SRC_DIR)
            dest_file = DEST_DIR / rel_path
            dest_file.parent.mkdir(parents=True, exist_ok=True)

            shutil.copy2(src_file, dest_file)
            log_info(f"Copied {src_file} -> {dest_file}")


@task
def html(ctx):
    """ Copy HTML files and update references to .min files """
    log_step("Processing HTML files")
    html_files = SRC_DIR.glob("**/*.html")

    for html_file in html_files:
        html_content = html_file.read_text(encoding="utf-8")

        html_content = re.sub(r'\.css(?=")', '.min.css', html_content)
        html_content = re.sub(r'\.js(?=")', '.min.js', html_content)

        rel_path = html_file.relative_to(SRC_DIR)
        dest_file = DEST_DIR / rel_path
        dest_file.parent.mkdir(parents=True, exist_ok=True)

        dest_file.write_text(html_content, encoding="utf-8")
        log_info(f"Processed {html_file} -> {dest_file}")


@task(pre=[clean, css_min, js_min, images, html])
def build(ctx):
    """ Run all tasks: clean, minify CSS/JS, copy images and HTML """
    log_step("Build complete!")
