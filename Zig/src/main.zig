const std = @import("std");

pub fn main() !void {
    var rand_impl = std.rand.DefaultPrng.init(10);
    const num = rand_impl.random().int(i32);
    std.debug.print("Random number: {}\tType: {}\n", .{ num, @TypeOf(num) });

    var random2 = std.Random.DefaultPrng.init(@as(u64, @abs(std.time.milliTimestamp())));
    const num2 = random2.random().int(i32);
    std.debug.print("Random number: {}\tType: {}\n", .{ num2, @TypeOf(num2) });

    var crypto_random = std.crypto.random;
    const num3 = crypto_random.int(i128);
    std.debug.print("Random number: {}\tType: {}\n", .{ num3, @TypeOf(num3) });
}
