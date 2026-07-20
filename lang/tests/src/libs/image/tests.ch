using std::Result;

@test
public func image_create_rgba_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(100, 50)
    if(image::image_width(&raw mut img) != 100) { env.error("width should be 100") }
    if(image::image_height(&raw mut img) != 50) { env.error("height should be 50") }
    if(image::image_channels(&raw mut img) != 4) { env.error("channels should be 4") }
    if(image::image_total_bytes(&raw mut img) != 20000) { env.error("total bytes should be 20000") }
}

@test
public func image_create_rgb_works(env : &mut TestEnv) {
    var img = image::image_create_rgb(80, 60)
    if(image::image_width(&raw mut img) != 80) { env.error("width") }
    if(image::image_height(&raw mut img) != 60) { env.error("height") }
    if(image::image_channels(&raw mut img) != 3) { env.error("channels") }
}

@test
public func image_create_gray_works(env : &mut TestEnv) {
    var img = image::image_create_gray(200, 100)
    if(image::image_channels(&raw mut img) != 1) { env.error("should be grayscale") }
}

@test
public func image_set_get_rgba_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(10, 10)
    var red = image::RGBA8.make(255, 0, 0, 255)
    var set_result = image::image_set_rgba(&raw mut img, 5, 5, red)
    if(set_result is Result.Err) { env.error("set should succeed"); return }
    var get_result = image::image_get_rgba(&raw mut img, 5, 5)
    if(get_result is Result.Err) { env.error("get should succeed"); return }
    var Ok(px) = get_result else unreachable
    if(px.r != 255) { env.error("red channel") }
    if(px.g != 0) { env.error("green channel") }
    if(px.b != 0) { env.error("blue channel") }
    if(px.a != 255) { env.error("alpha channel") }
}

@test
public func image_fill_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(10, 10)
    var white = image::RGBA8.make(255, 255, 255, 255)
    image::image_fill(&raw mut img, white)
    var px = image::image_get_rgba(&raw mut img, 0, 0)
    if(px is Result.Err) { env.error("get should succeed"); return }
    var Ok(c) = px else unreachable
    if(c.r != 255) { env.error("filled red") }
    if(c.g != 255) { env.error("filled green") }
}

@test
public func image_copy_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(10, 10)
    var red = image::RGBA8.make(255, 0, 0, 255)
    image::image_set_rgba(&raw mut img, 3, 3, red)
    var copy = image::image_copy(&raw mut img)
    if(image::image_width(&raw mut copy) != 10) { env.error("copy width") }
    var px = image::image_get_rgba(&raw mut copy, 3, 3)
    if(px is Result.Err) { env.error("copied pixel should match"); return }
    var Ok(c) = px else unreachable
    if(c.r != 255) { env.error("copied pixel should match") }
}

@test
public func image_crop_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(20, 20)
    var cropped_result = image::image_crop(&raw mut img, 2, 2, 10, 10)
    if(cropped_result is Result.Err) { env.error("crop should succeed"); return }
    var Ok(cropped) = cropped_result else unreachable
    if(image::image_width(&raw mut cropped) != 10) { env.error("cropped width") }
    if(image::image_height(&raw mut cropped) != 10) { env.error("cropped height") }
}

@test
public func image_flip_h_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(10, 10)
    var red = image::RGBA8.make(255, 0, 0, 255)
    image::image_set_rgba(&raw mut img, 0, 5, red)
    image::image_flip_h(&raw mut img)
    var px = image::image_get_rgba(&raw mut img, 9, 5)
    if(px is Result.Err) { env.error("flipped horizontally"); return }
    var Ok(c) = px else unreachable
    if(c.r != 255) { env.error("flipped horizontally") }
}

@test
public func image_rotate90_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(20, 10)
    var rotated = image::image_rotate90(&raw mut img)
    if(image::image_width(&raw mut rotated) != 10) { env.error("rotated width") }
    if(image::image_height(&raw mut rotated) != 20) { env.error("rotated height") }
}

@test
public func image_line_draws(env : &mut TestEnv) {
    var img = image::image_create_rgba(100, 100)
    var red = image::RGBA8.make(255, 0, 0, 255)
    image::image_line(&raw mut img, 0, 0, 99, 99, red)
    var px = image::image_get_rgba(&raw mut img, 50, 50)
    if(px is Result.Err) { env.error("line should pass through center"); return }
    var Ok(c) = px else unreachable
    if(c.r != 255) { env.error("line should pass through center") }
}

@test
public func image_fill_rect_works(env : &mut TestEnv) {
    var img = image::image_create_rgba(100, 100)
    var blue = image::RGBA8.make(0, 0, 255, 255)
    image::image_fill_rect(&raw mut img, 10, 10, 20, 20, blue)
    var px = image::image_get_rgba(&raw mut img, 15, 15)
    if(px is Result.Err) { env.error("filled rect should have blue"); return }
    var Ok(c) = px else unreachable
    if(c.b != 255) { env.error("filled rect should have blue") }
    var outside = image::image_get_rgba(&raw mut img, 5, 5)
    if(outside is Result.Err) { env.error("outside should be empty"); return }
    var Ok(o) = outside else unreachable
    if(o.b != 0) { env.error("outside should be empty") }
}

@test
public func image_circle_draws(env : &mut TestEnv) {
    var img = image::image_create_rgba(100, 100)
    var red = image::RGBA8.make(255, 0, 0, 255)
    image::image_circle(&raw mut img, 50, 50, 20, red)
    var px = image::image_get_rgba(&raw mut img, 50, 30)
    if(px is Result.Err) { env.error("circle top should have pixel"); return }
    var Ok(c) = px else unreachable
    if(c.r != 255) { env.error("circle top should have pixel") }
}

@test
public func image_blit_works(env : &mut TestEnv) {
    var dst = image::image_create_rgba(100, 100)
    var src = image::image_create_rgba(10, 10)
    var red = image::RGBA8.make(255, 0, 0, 255)
    image::image_set_rgba(&raw mut src, 0, 0, red)
    var result = image::image_blit(&raw mut dst, &raw mut src, 5, 5)
    if(result is Result.Err) { env.error("blit should succeed"); return }
    var px = image::image_get_rgba(&raw mut dst, 5, 5)
    if(px is Result.Err) { env.error("blitted pixel"); return }
    var Ok(c) = px else unreachable
    if(c.r != 255) { env.error("blitted pixel") }
}
