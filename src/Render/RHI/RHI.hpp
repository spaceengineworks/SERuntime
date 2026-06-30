#ifndef RHI_H
#define RHI_H

namespace SE
{

enum SeResultInternals
{
    SE_SUCCESS = 0,
    SE_FAILED  = 1
};

using SeResult = SeResultInternals;

class RHI
{
   public:
    struct Config
    {
    };

    virtual ~RHI()                                    = default;
    virtual SeResult initConfig(const Config& config) = 0;
};
}  // namespace SE

#endif /* RHI_H */