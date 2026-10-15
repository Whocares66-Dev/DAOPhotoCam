# The guide

The player's guide, a Jekyll site in `/docs` beside the source, as the other DAO repositories' are (Just the Docs v0.12.0). It is published to GitHub Pages by `.github/workflows/docs.yml` on every push to master that touches it. Developer notes are in `/dev`.

## Preview

```sh
cd docs
bundle install
bundle exec jekyll serve
```

Then open `http://127.0.0.1:4000/DAOPhotoCam/`.

## Looks

Black and white (the user, 2026-10-15: "use a black / white theme"): `_sass/color_schemes/photocam.scss` has the colours, `_sass/custom/custom.scss` the rest (links underlined, pictures unframed, keys drawn as keys, tables as rows). The title's camera is `assets/camera-icon.png` and the favicon `assets/favicon.ico`, both from `tools/camera_icon.py`'s drawing.

## Editing

`index.md` and the pages beside it, written by hand; each page's `nav_order` is its place in the sidebar. Clear and short (the user: "be clear and succinct"). One line a paragraph, as everywhere in this repository.

## Pictures

`assets/img/` holds the pictures, from the game's own frames at 1600 x 1000, in the `photocam` instance on 2026-10-15:

| Picture | From |
|---|---|
| `frozen-fight.jpg` | a fight with two wolves, paused by photo cam, the view placed with `photocam_pose` |
| `capture.jpg` | the user's own Capture |
| `photo-cam-on.jpg`, `panel.png` | photo cam on; the panel cropped and doubled |
| `top-bar.png` | the game's own view, its top bar cropped and doubled |
