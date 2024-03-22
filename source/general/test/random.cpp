#include <gtest/gtest.h>

#include "sung/general/random.hpp"


namespace {

    TEST(Random, RealNumberGenerator) {
        using T = float;
        constexpr int ITERATIONS = 10000;
        double weighted_sum = 0;
        sung::RandomRealNumGenerator<T> gen((T)-0.8, (T)1.3);

        for (int i = 0; i < ITERATIONS; ++i) {
            const auto val = gen.gen();
            EXPECT_GE(val, gen.lower_bound());
            EXPECT_LE(val, gen.upper_bound());
            weighted_sum += val / ITERATIONS;
        }

        const auto expected_mean = (gen.upper_bound() + gen.lower_bound()) *
                                   0.5;
        EXPECT_NEAR(weighted_sum, expected_mean, 1e-2);
    }

}  // namespace


int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
