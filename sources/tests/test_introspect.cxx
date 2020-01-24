#include <iostream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "apophenic/Introspect.hxx"


class Alpha;
typedef ap::insp::Introspected<Alpha, unsigned[5], int, int *, const std::string, std::string, bool> AlphaParent;

class Alpha : public AlphaParent
{
public:
	Alpha() = default;
    Alpha(int b, int * c, std::string d, std::string e, bool f) : AlphaParent(b, c, d, e, f) {}
};


namespace ap
{
namespace insp
{

template<> const char * const NamedMember<Alpha,5,unsigned[5]>::kNAME = "First";
template<> const char * const NamedMember<Alpha,4,int>::kNAME = "Second";
template<> const char * const NamedMember<Alpha,3,int *>::kNAME = "Third";
template<> const char * const NamedMember<Alpha,2,const std::string>::kNAME = "Fourth";
template<> const char * const NamedMember<Alpha,1,std::string>::kNAME = "Fifth";
template<> const char * const NamedMember<Alpha,0,bool>::kNAME = "Sixth";

}
}


TEST(IntrospectFixture, constructor)
{
	int c = -37;
	Alpha alpha(5, &c, "Hello", "Goodbye", true);
	Alpha beta;
}


TEST(IntrospectFixture, read_member)
{
	int c = -37;
	Alpha alpha(5, &c, "Hello", "Goodbye", true);

	EXPECT_TRUE(alpha.get<bool>(0));
	EXPECT_EQ(std::string("Goodbye"), alpha.get<std::string>(1));
	EXPECT_EQ(std::string("Hello"), alpha.get<const std::string>(2));
	EXPECT_EQ(&c, alpha.get<int *>(3));
	EXPECT_EQ(5, alpha.get<int>(4));

	EXPECT_TRUE(alpha.get<bool>("Sixth"));
	EXPECT_EQ(std::string("Goodbye"), alpha.get<std::string>("Fifth"));
	EXPECT_EQ(std::string("Hello"), alpha.get<const std::string>("Fourth"));
	EXPECT_EQ(&c, alpha.get<int *>("Third"));
	EXPECT_EQ(5, alpha.get<int>("Second"));
	EXPECT_TRUE(nullptr != alpha.get<unsigned[5]>("First"));

	const Alpha beta(5, &c, "Hello", "Goodbye", true);

	EXPECT_TRUE(beta.get<bool>(0));
	EXPECT_EQ(std::string("Goodbye"), beta.get<std::string>(1));
	EXPECT_EQ(std::string("Hello"), beta.get<const std::string>(2));
	EXPECT_EQ(&c, beta.get<int *>(3));
	EXPECT_EQ(5, beta.get<int>(4));

	EXPECT_TRUE(beta.get<bool>("Sixth"));
	EXPECT_EQ(std::string("Goodbye"), beta.get<std::string>("Fifth"));
	EXPECT_EQ(std::string("Hello"), beta.get<const std::string>("Fourth"));
	EXPECT_EQ(&c, beta.get<int *>("Third"));
	EXPECT_EQ(5, beta.get<int>("Second"));
	EXPECT_TRUE(nullptr != beta.get<unsigned[5]>("First"));
}


TEST(IntrospectFixture, modify_member)
{
	int c = -37;
	Alpha alpha(5, &c, "Hello", "Goodbye", true);

	EXPECT_TRUE(alpha.get<bool>(0));
	EXPECT_EQ(std::string("Goodbye"), alpha.get<std::string>(1));
	EXPECT_EQ(std::string("Hello"), alpha.get<const std::string>(2));
	EXPECT_EQ(&c, alpha.get<int *>(3));
	EXPECT_EQ(5, alpha.get<int>(4));

	alpha.get<bool>("Sixth") = false;
	alpha.get<std::string>("Fifth") = "Adios";
	int k = 457;
	alpha.get<int *>("Third") = &k;
	alpha.get<int>("Second") = -555;

	for(unsigned i = 0; i < 5; ++i)
	{
		alpha.get<unsigned[5]>("First")[i] = i << 2;
	}

	EXPECT_FALSE(alpha.get<bool>("Sixth"));
	EXPECT_EQ(std::string("Adios"), alpha.get<std::string>("Fifth"));
	EXPECT_EQ(&k, alpha.get<int *>("Third"));
	EXPECT_EQ(-555, alpha.get<int>("Second"));

	for(unsigned i = 0; i < 5; ++i)
	{
		EXPECT_EQ(i, alpha.get<unsigned[5]>("First")[i] >> 2);
	}
}


TEST(IntrospectFixture, errors)
{
	int c = -37;
	Alpha alpha(5, &c, "Hello", "Goodbye", true);

	EXPECT_THROW(alpha.get<int>(6), ap::insp::BadRank);
	EXPECT_THROW(alpha.get<int>("Fist"), ap::insp::BadName);
	EXPECT_THROW(alpha.get<unsigned>("First"), ap::insp::BadType);
}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
