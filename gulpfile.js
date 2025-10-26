const gulp = require('gulp');
const cleanCSS = require('gulp-clean-css');
const esbuild = require('esbuild');
const rename = require('gulp-rename');
const replace = require('gulp-replace');
const sass = require('gulp-sass')(require('sass'));
const fs = require('fs');
const path = require('path');

const paths = {
    scss: {
        src: 'src/www/**/*.scss',
        dest: 'data/www'
    },
    js: {
        src: 'src/www/**/*.js',
        dest: 'data/www'
    }
};

function clean(done) {
    if (fs.existsSync('data/www')) {
        fs.rmSync('data/www', {recursive: true, force: true});
    }
    done();
}

function scss() {
    return gulp.src(paths.scss.src)
        .pipe(sass({includePaths: ['node_modules']}).on('error', sass.logError))
        .pipe(cleanCSS())
        .pipe(rename({suffix: '.min', extname: '.css'}))
        .pipe(gulp.dest(paths.scss.dest));
}

async function js() {
    const entryPoints = [path.resolve('src/www/assets/js/main.js')];

    await esbuild.build({
        entryPoints,
        bundle: true,
        minify: true,
        format: 'iife',
        target: 'es2015',
        outfile: 'data/www/assets/js/main.min.js'
    });
}

const build = gulp.series(clean, gulp.parallel(scss, js));

exports.clean = clean;
exports.scss = scss;
exports.js = js;
exports.build = build;
exports.default = build;
