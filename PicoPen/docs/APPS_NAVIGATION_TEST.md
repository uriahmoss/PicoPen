# Apps Navigation Hardware Test

This test covers the categorized Apps launcher, persistent favorites, full-list
scrolling, and app information added after the host-centric workflow milestone.

## Flash

Flash `build/pico2w-debug/picopen_os.uf2` using the already verified bootloader
and primary-slot procedure. Do not replace the stage-1 bootloader for this test.

## Test

1. Open **Apps** and verify the header starts on **ALL**.
2. Use Left and Right. Verify only non-empty views are offered.
3. Highlight an app and press `F`. Cycle to **FAVORITES** and verify it appears.
4. Press `H` on that app. Verify its ID, category, source, status, provider, and
   capabilities are shown. Press Escape and verify the same app remains selected.
5. Leave Apps, return, and verify its view and selection are remembered.
6. Reboot PicoPen and verify the view, selection, and favorite remain saved.
7. Remove the favorite with `F`. Verify an empty Favorites view is skipped during
   subsequent Left/Right navigation.
8. If enough SD app descriptors are present, scroll beyond nine entries and verify
   the list follows the selection without trapping it off-screen.

## Pass criteria

- Navigation remains responsive and Escape preserves context.
- Empty categories are skipped.
- Favorites and launcher position survive a reboot.
- The information screen accurately describes both built-in and SD-discovered apps.
- Existing host actions and inspectors still launch normally.
